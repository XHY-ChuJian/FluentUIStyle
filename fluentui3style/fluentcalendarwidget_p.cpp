#include "fluentcalendarwidget_p.h"

#include <QApplication>
#include <QCalendarWidget>
#include <QDate>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHoverEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>
#include <QSpinBox>
#include <QStyleOptionToolButton>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include "fluentui3colors.h"
#include "fluentui3style.h"
#include "fluentui3styleproperties.h"
#include "qstyle.h"

namespace {
inline bool isDarkMode( const QWidget* widget )
{
    if ( qApp && qApp->property( "_q_colorscheme" ).isValid() )
    {
        return qApp->property( "_q_colorscheme" ).toInt() == 1;
    }
    if ( widget )
    {
        return widget->palette().color( QPalette::Window ).lightness() < 128;
    }
    return false;
}

// 根据 Qt 原生 QCalendarModel 算法从 (row, col) 还原当前单元格对应的 QDate
inline QDate cellDateFromIndex( const QCalendarWidget* calendar, const QModelIndex& index )
{
    if ( !calendar || !index.isValid() )
    {
        return QDate();
    }
    int row    = index.row();
    int column = index.column();

    const int firstRow    = ( calendar->horizontalHeaderFormat() != QCalendarWidget::NoHorizontalHeader ) ? 1 : 0;
    const int firstColumn = ( calendar->verticalHeaderFormat() != QCalendarWidget::NoVerticalHeader ) ? 1 : 0;

    if ( row < firstRow || column < firstColumn )
    {
        return QDate();
    }

    const int shownYear          = calendar->yearShown();
    const int shownMonth         = calendar->monthShown();
    const Qt::DayOfWeek firstDay = calendar->firstDayOfWeek();

    // 参考日期：当月 1 号
    QDate refDate( shownYear, shownMonth, 1 );
    if ( !refDate.isValid() )
    {
        return QDate();
    }

    auto columnForDayOfWeek = [ firstDay, firstColumn ]( Qt::DayOfWeek day ) -> int
    {
        int col = (int)day - (int)firstDay;
        if ( col < 0 )
        {
            col += 7;
        }
        return col + firstColumn;
    };

    // 当月第一天所在的列
    const int colForFirstOfMonth =
        ( columnForDayOfWeek( static_cast<Qt::DayOfWeek>( refDate.dayOfWeek() ) ) - ( refDate.day() % 7 ) + 8 ) % 7;

    if ( colForFirstOfMonth - firstColumn < 1 )
    {
        row -= 1;
    }

    const int requestedDay = 7 * ( row - firstRow ) + column - colForFirstOfMonth - refDate.day() + 1;
    return refDate.addDays( requestedDay );
}

// 日期单元格代理：严格遵循 WinUI 3 正圆形选框与高对比排版
class FluentCalendarDelegate : public QStyledItemDelegate
{
public:
    explicit FluentCalendarDelegate( QCalendarWidget* calendar, QObject* parent = nullptr )
        : QStyledItemDelegate( parent )
        , m_calendar( calendar )
    {
    }

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        if ( !m_calendar )
        {
            QStyledItemDelegate::paint( painter, option, index );
            return;
        }

        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );
        painter->setRenderHint( QPainter::TextAntialiasing );

        const bool isDark  = isDarkMode( m_calendar );
        const auto& colors = isDark ? WINUI3ColorsDark : WINUI3ColorsLight;

        const QDate cellDate  = cellDateFromIndex( m_calendar, index );
        const bool isToday    = cellDate.isValid() && ( cellDate == QDate::currentDate() );
        const bool isSelected = ( option.state & QStyle::State_Selected )
                                || ( cellDate.isValid() && cellDate == m_calendar->selectedDate() );
        const bool isCurrentMonth =
            cellDate.isValid() && ( cellDate.month() == m_calendar->monthShown() ) && ( cellDate.year() == m_calendar->yearShown() );
        const bool isHovered = ( option.state & QStyle::State_MouseOver );
        const bool isEnabled = ( option.state & QStyle::State_Enabled );
        const bool hasFocus  = ( option.state & QStyle::State_HasFocus );

        const QRectF r = option.rect;
        // 在单元格中心取正圆形（WinUI 3 DayItem 规范）
        const qreal diameter = qMax<qreal>( 24.0, qMin( r.width(), r.height() ) - 6.0 );
        const QRectF circleRect( r.center().x() - diameter / 2.0, r.center().y() - diameter / 2.0, diameter, diameter );

#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
        const QColor accentColor = option.palette.accent().color();
#else
        const QColor accentColor = option.palette.highlight().color();
#endif

        if ( isSelected )
        {
            // 1. 选中态：实心 Accent 强调色正圆形
            QColor selBg = accentColor;
            if ( isHovered )
            {
                selBg = isDark ? selBg.lighter( 112 ) : selBg.darker( 108 );
            }
            painter->setPen( Qt::NoPen );
            painter->setBrush( selBg );
            painter->drawEllipse( circleRect );
        }
        else if ( isToday )
        {
            // 2. 今天（未选中）：1.5px Accent 强调色圆环，悬停带浅底
            if ( isHovered && isEnabled )
            {
                painter->setBrush( colors[ WINUI3Color::subtleHighlightColor ] );
            }
            else
            {
                painter->setBrush( Qt::NoBrush );
            }
            painter->setPen( QPen( accentColor, 1.5 ) );
            painter->drawEllipse( circleRect.adjusted( 0.75, 0.75, -0.75, -0.75 ) );
        }
        else if ( isHovered && isEnabled )
        {
            // 3. 普通悬停：浅灰半透明正圆形
            painter->setPen( Qt::NoPen );
            QColor hoverBg = colors[ WINUI3Color::subtleHighlightColor ];
            if ( !isCurrentMonth )
            {
                hoverBg.setAlpha( qRound( hoverBg.alpha() * 0.6 ) );
            }
            painter->setBrush( hoverBg );
            painter->drawEllipse( circleRect );
        }

        // 绘制日期文本
        const QString text = index.data( Qt::DisplayRole ).toString();
        if ( !text.isEmpty() )
        {
            QColor textColor;
            QFont font = option.font;
            font.setPixelSize( 14 );

            const bool isWeekend = cellDate.isValid() && ( cellDate.dayOfWeek() == Qt::Saturday || cellDate.dayOfWeek() == Qt::Sunday );

            if ( isSelected )
            {
                textColor = colors[ WINUI3Color::textOnAccentPrimary ];
                font.setWeight( QFont::DemiBold );
            }
            else if ( !isEnabled )
            {
                textColor = colors[ WINUI3Color::textDisabled ];
            }
            else if ( !isCurrentMonth )
            {
                // 非当月日期淡化（WinUI 3 Out-of-month）
                if ( isWeekend )
                {
                    QColor dimAccent = accentColor;
                    dimAccent.setAlpha( isDark ? 120 : 130 );
                    textColor = dimAccent;
                }
                else
                {
                    textColor = isDark ? QColor( 255, 255, 255, 85 ) : QColor( 0, 0, 0, 85 );
                }
                font.setWeight( QFont::Normal );
            }
            else if ( isToday )
            {
                textColor = isDark ? QColor( 255, 255, 255, 255 ) : QColor( 0, 0, 0, 240 );
                font.setWeight( QFont::DemiBold );
            }
            else if ( isWeekend )
            {
                textColor = accentColor;
                font.setWeight( QFont::Normal );
            }
            else
            {
                textColor = isDark ? QColor( 255, 255, 255, 235 ) : QColor( 0, 0, 0, 225 );
                font.setWeight( QFont::Normal );
            }

            painter->setFont( font );
            painter->setPen( textColor );
            painter->drawText( circleRect, Qt::AlignCenter, text );
        }

        painter->restore();
    }

private:
    QPointer<QCalendarWidget> m_calendar;
};

// 星期表头代理：WinUI 3 极简无边框，SemiBold 居中
class FluentCalendarHeaderDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        painter->save();
        painter->setRenderHint( QPainter::Antialiasing );
        painter->setRenderHint( QPainter::TextAntialiasing );

        const bool isDark = isDarkMode( nullptr );

        QFont font = option.font;
        font.setPixelSize( 13 );
        font.setWeight( QFont::DemiBold );

        // 默认周一为第一列，第 5、6 列为周六、周日
        const int col           = index.column();
        const bool isWeekendCol = ( col == 5 || col == 6 );

#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
        const QColor accentColor = option.palette.accent().color();
#else
        const QColor accentColor = option.palette.highlight().color();
#endif

        QColor textColor;
        if ( isWeekendCol )
        {
            textColor = accentColor;
        }
        else
        {
            textColor = isDark ? QColor( 255, 255, 255, 240 ) : QColor( 0, 0, 0, 220 );
        }

        painter->setFont( font );
        painter->setPen( textColor );
        painter->drawText( option.rect, Qt::AlignCenter, index.data( Qt::DisplayRole ).toString() );

        painter->restore();
    }
};

// 事件过滤器：为 QCalendarWidget 绘制 PE_FluentFlyoutSurface
// 卡片表面，并将按钮委托给 FluentUI3Style 标准渲染
class FluentCalendarEventFilter : public QObject
{
public:
    explicit FluentCalendarEventFilter( QCalendarWidget* calendar )
        : QObject( calendar )
        , m_calendar( calendar )
    {
    }

protected:
    bool eventFilter( QObject* watched, QEvent* event ) override
    {
        if ( watched == m_calendar )
        {
            if ( event->type() == QEvent::Paint )
            {
                QPainter painter( m_calendar );
                painter.save();
                painter.fillRect( m_calendar->rect(), QColor( 0, 0, 0, 0 ) );
                painter.setRenderHint( QPainter::Antialiasing );

                const int shadowReserve = FlyoutShadowBorderWidth;
                const QRect panelRect   = m_calendar->rect().adjusted( shadowReserve, shadowReserve, -shadowReserve, -shadowReserve );

                QStyleOption opt;
                opt.initFrom( m_calendar );
                opt.rect = panelRect;

                qApp->style()->drawPrimitive( static_cast<QStyle::PrimitiveElement>( PE_FluentFlyoutSurface ), &opt, &painter, m_calendar );
                painter.restore();
            }
        }
        else if ( auto* btn = qobject_cast<QToolButton*>( watched ) )
        {
            const bool isPrev  = ( btn->objectName() == "qt_calendar_prevmonth" );
            const bool isNext  = ( btn->objectName() == "qt_calendar_nextmonth" );
            const bool isMonth = ( btn->objectName() == "qt_calendar_monthbutton" );
            const bool isYear  = ( btn->objectName() == "qt_calendar_yearbutton" );

            if ( ( isPrev || isNext || isMonth || isYear ) && event->type() == QEvent::Paint )
            {
                QPainter painter( btn );

                // 纠正 QCalToolButton 强制篡改 HighlightedText 的问题，直接调用 Style
                // 的标准绘制
                QStyleOptionToolButton opt;
                opt.initFrom( btn );
                opt.text        = btn->text();
                opt.icon        = btn->icon();
                opt.iconSize    = btn->iconSize();
                opt.subControls = QStyle::SC_ToolButton;

                if ( isMonth )
                {
                    opt.features |= QStyleOptionToolButton::HasMenu;
                }

                if ( isPrev || isNext )
                {
                    const QColor iconColor = qApp->palette().color( QPalette::Text );
                    if ( auto* style = qobject_cast<FluentUI3Style*>( qApp->style() ) )
                    {
                        opt.icon     = style->fluentIcon( QChar( isPrev ? 0xEDDB : 0xEDDC ), iconColor );
                        opt.iconSize = btn->iconSize().isValid() ? btn->iconSize() : QSize( 14, 14 );
                    }
                }

                if ( btn->autoRaise() )
                {
                    opt.state |= QStyle::State_AutoRaise;
                }
                if ( btn->isDown() )
                {
                    opt.state |= QStyle::State_Sunken;
                }
                if ( btn->underMouse() )
                {
                    opt.state |= QStyle::State_MouseOver;
                }

                qApp->style()->drawComplexControl( QStyle::CC_ToolButton, &opt, &painter, btn );

                return true;
            }
        }

        return QObject::eventFilter( watched, event );
    }

private:
    QPointer<QCalendarWidget> m_calendar;
};
}  // namespace

void polishCalendarWidget( QCalendarWidget* calendar )
{
    if ( !calendar )
    {
        return;
    }

    if ( calendar->property( "_q_fluent_calendar_polished" ).toBool() )
    {
        calendar->update();
        return;
    }
    calendar->setProperty( "_q_fluent_calendar_polished", true );
    calendar->setAutoFillBackground( false );
    calendar->setAttribute( Qt::WA_TranslucentBackground, true );
    calendar->setAttribute( Qt::WA_OpaquePaintEvent, false );

    // 推荐适度增大尺寸，保证 WinUI 3 开阔大气的布局比例与阴影预留
    calendar->setMinimumSize( 320 + FlyoutShadowBorderWidth * 2, 360 + FlyoutShadowBorderWidth * 2 );

    auto* eventFilter = new FluentCalendarEventFilter( calendar );
    calendar->installEventFilter( eventFilter );

    // 调整内部主布局内边距：FlyoutShadowBorderWidth(6px 阴影) + 8px 内部呼吸间距
    if ( auto* mainLayout = calendar->layout() )
    {
        const int m = FlyoutShadowBorderWidth + 8;
        mainLayout->setContentsMargins( m, m, m, m );
        mainLayout->setSpacing( 6 );
    }

    // 定制顶部导航栏及其子控件
    auto* prevBtn  = calendar->findChild<QToolButton*>( QStringLiteral( "qt_calendar_prevmonth" ) );
    auto* nextBtn  = calendar->findChild<QToolButton*>( QStringLiteral( "qt_calendar_nextmonth" ) );
    auto* monthBtn = calendar->findChild<QToolButton*>( QStringLiteral( "qt_calendar_monthbutton" ) );
    auto* yearBtn  = calendar->findChild<QToolButton*>( QStringLiteral( "qt_calendar_yearbutton" ) );
    auto* yearEdit = calendar->findChild<QSpinBox*>( QStringLiteral( "qt_calendar_yearedit" ) );
    auto* navBar   = calendar->findChild<QWidget*>( QStringLiteral( "qt_calendar_navigationbar" ) );

    // 定制翻页按钮（WinUI 3 顶部实心三角 CaretUpSolid8 / CaretDownSolid8）
    if ( prevBtn )
    {
        prevBtn->setAutoRaise( true );
        prevBtn->setFixedSize( 32, 32 );
        prevBtn->setIconSize( QSize( 14, 14 ) );
        prevBtn->installEventFilter( eventFilter );
    }
    if ( nextBtn )
    {
        nextBtn->setAutoRaise( true );
        nextBtn->setFixedSize( 32, 32 );
        nextBtn->setIconSize( QSize( 14, 14 ) );
        nextBtn->installEventFilter( eventFilter );
    }

    // monthBtn 与 yearBtn 均是 QCalToolButton(QToolButton)，统一委托给
    // FluentUI3Style 渲染
    if ( monthBtn )
    {
        monthBtn->setAutoRaise( true );
        monthBtn->setFixedHeight( 32 );
        QFont font = monthBtn->font();
        font.setPixelSize( 15 );
        font.setWeight( QFont::DemiBold );
        monthBtn->setFont( font );
        monthBtn->installEventFilter( eventFilter );
    }

    if ( yearBtn )
    {
        yearBtn->setAutoRaise( true );
        yearBtn->setFixedHeight( 32 );
        QFont font = yearBtn->font();
        font.setPixelSize( 15 );
        font.setWeight( QFont::DemiBold );
        yearBtn->setFont( font );
        yearBtn->installEventFilter( eventFilter );
    }

    if ( yearEdit )
    {
        yearEdit->setButtonSymbols( QAbstractSpinBox::UpDownArrows );
        QFont font = yearEdit->font();
        font.setPixelSize( 15 );
        font.setWeight( QFont::DemiBold );
        yearEdit->setFont( font );
        yearEdit->setMinimumWidth( 120 );
        yearEdit->setFixedHeight( 32 );
    }

    // 重构顶部导航栏为标准 WinUI 3 布局：[月份] [年份] [弹性拉伸] [上一月]
    // [下一月] 保证月年紧挨并排在最左侧，弹性弹簧位于年份与翻页按钮之间
    if ( navBar )
    {
        navBar->setAutoFillBackground( false );
        navBar->setAttribute( Qt::WA_TranslucentBackground, true );
        navBar->setFixedHeight( 42 );

        if ( auto* navLay = qobject_cast<QBoxLayout*>( navBar->layout() ) )
        {
            navLay->setContentsMargins( 4, 0, 4, 0 );
            navLay->setSpacing( 4 );

            if ( prevBtn )
            {
                navLay->removeWidget( prevBtn );
            }
            if ( monthBtn )
            {
                navLay->removeWidget( monthBtn );
            }
            if ( yearBtn )
            {
                navLay->removeWidget( yearBtn );
            }
            if ( nextBtn )
            {
                navLay->removeWidget( nextBtn );
            }

            // 1. 最左侧紧挨插入 [月份] 与 [年份]（中间无弹簧）
            if ( monthBtn )
            {
                navLay->insertWidget( 0, monthBtn );
            }
            if ( yearBtn )
            {
                navLay->insertWidget( 1, yearBtn );
            }

            // 2. 弹簧之后，在最右侧追加 [上一月] 与 [下一月]
            if ( prevBtn )
            {
                navLay->addWidget( prevBtn );
            }
            if ( nextBtn )
            {
                navLay->addWidget( nextBtn );
            }
        }
    }

    // 定制内部表格视图 QTableView
    if ( auto* view = calendar->findChild<QTableView*>( QStringLiteral( "qt_calendar_calendarview" ) ) )
    {
        view->setFrameShape( QFrame::NoFrame );
        view->setShowGrid( false );
        view->setMouseTracking( true );
        view->setAttribute( Qt::WA_TranslucentBackground, true );
        view->setAttribute( Qt::WA_OpaquePaintEvent, false );
        view->setAutoFillBackground( false );
        if ( view->viewport() )
        {
            view->viewport()->setAttribute( Qt::WA_TranslucentBackground, true );
            view->viewport()->setAttribute( Qt::WA_OpaquePaintEvent, false );
            view->viewport()->setAutoFillBackground( false );
        }
        view->setSelectionMode( QAbstractItemView::SingleSelection );

        if ( auto* hHeader = view->horizontalHeader() )
        {
            hHeader->setSectionResizeMode( QHeaderView::Stretch );
            hHeader->setHighlightSections( false );
            hHeader->setDefaultAlignment( Qt::AlignCenter );
            hHeader->setFixedHeight( 36 );
            hHeader->setItemDelegate( new FluentCalendarHeaderDelegate( hHeader ) );
        }

        if ( auto* vHeader = view->verticalHeader() )
        {
            vHeader->setSectionResizeMode( QHeaderView::Stretch );
            vHeader->setHighlightSections( false );
            vHeader->hide();  // WinUI 3 默认不展示周数列
        }

        view->setItemDelegate( new FluentCalendarDelegate( calendar, view ) );
    }
}
