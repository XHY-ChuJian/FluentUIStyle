#pragma once

#include <QColor>
#include <QDateTime>
#include <QList>
#include <QListView>
#include <QObject>
#include <QPalette>
#include <QString>

#include "exwidgets_global.h"
#include "exwidgetsmacros.h"

class QEvent;
class QHideEvent;
class QShowEvent;
class QVariantAnimation;
class ExTimelineDelegate;
class ExTimelineModel;

class EXWIDGETS_EXPORT ExTimelineEvent final : public QObject
{
    Q_OBJECT

public:
    enum Status
    {
        Normal,
        Completed,
        Current,
        Pending,
        Warning,
        Error
    };
    Q_ENUM( Status )

    enum Placement
    {
        Automatic,
        LeftSide,
        RightSide
    };
    Q_ENUM( Placement )

    // 事件发生的时间；timeText 为空时按照所属时间轴的格式显示。
    EXWIDGETS_DECLARE_PROPERTY( QDateTime, timestamp, timestamp, setTimestamp, QDateTime() )

    // 自定义时间文本，非空时优先于 timestamp。
    EXWIDGETS_DECLARE_PROPERTY( QString, timeText, timeText, setTimeText, QString() )

    // 事件标题。
    EXWIDGETS_DECLARE_PROPERTY( QString, title, title, setTitle, QString() )

    // 事件的补充描述。
    EXWIDGETS_DECLARE_PROPERTY( QString, description, description, setDescription, QString() )

    // 事件状态，用于决定默认节点颜色和图形。
    EXWIDGETS_DECLARE_PROPERTY( Status, status, status, setStatus, Normal )

    // 自定义节点颜色；无效颜色表示根据 status 和调色板自动选择。
    EXWIDGETS_DECLARE_PROPERTY( QColor, color, color, setColor, QColor() )

    // 节点内的 Fluent 字体图标字符；为空时使用状态默认图形。
    EXWIDGETS_DECLARE_PROPERTY( QString, icon, icon, setIcon, QString() )

    // 交错布局下单个事件的内容位置；水平时间轴中 Left/Right 对应上/下。
    EXWIDGETS_DECLARE_PROPERTY( Placement, placement, placement, setPlacement, Automatic )

    explicit ExTimelineEvent( QObject* parent = nullptr );
    ExTimelineEvent( const QDateTime& timestamp,
                     const QString& title,
                     const QString& description = QString(),
                     Status status              = Normal,
                     QObject* parent            = nullptr );

    Q_SIGNAL void itemChanged();
};

class EXWIDGETS_EXPORT ExTimeline final : public QListView
{
    Q_OBJECT

public:
    enum LayoutMode
    {
        ContentOnRight,
        ContentOnLeft,
        Alternating,
        AlternatingReverse
    };
    Q_ENUM( LayoutMode )

    // 时间轴方向；水平模式下左/右布局分别映射为上/下布局。
    EXWIDGETS_DECLARE_PROPERTY( Qt::Orientation, orientation, orientation, setOrientation, Qt::Vertical )

    // 事件内容相对时间轴的位置。
    EXWIDGETS_DECLARE_PROPERTY( LayoutMode, layoutMode, layoutMode, setLayoutMode, ContentOnRight )

    // 是否反转事件的显示顺序，但不改变 events() 的存储顺序。
    EXWIDGETS_DECLARE_PROPERTY( bool, reverse, isReverse, setReverse, false )

    // 是否显示事件时间。
    EXWIDGETS_DECLARE_PROPERTY( bool, timestampVisible, isTimestampVisible, setTimestampVisible, true )

    // 是否显示事件描述。
    EXWIDGETS_DECLARE_PROPERTY( bool, descriptionVisible, isDescriptionVisible, setDescriptionVisible, true )

    // QDateTime 转换为文字时使用的格式。
    EXWIDGETS_DECLARE_PROPERTY( QString, timestampFormat, timestampFormat, setTimestampFormat, QStringLiteral( "yyyy-MM-dd HH:mm" ) )

    // 时间列宽度，交错布局时表示时间侧可用宽度的上限。
    EXWIDGETS_DECLARE_PROPERTY( int, timestampWidth, timestampWidth, setTimestampWidth, 116 )

    // 节点直径。
    EXWIDGETS_DECLARE_PROPERTY( int, nodeSize, nodeSize, setNodeSize, 14 )

    // 时间轴连接线宽度。
    EXWIDGETS_DECLARE_PROPERTY( qreal, lineWidth, lineWidth, setLineWidth, 2.0 )

    // 垂直时间轴中相邻事件内容之间的间距。
    EXWIDGETS_DECLARE_PROPERTY( int, itemSpacing, itemSpacing, setItemSpacing, 18 )

    // 水平时间轴中每个事件占用的宽度。
    EXWIDGETS_DECLARE_PROPERTY( int, horizontalItemWidth, horizontalItemWidth, setHorizontalItemWidth, 240 )

    // 时间轴内容与视口边缘之间的距离。
    EXWIDGETS_DECLARE_PROPERTY( int, contentPadding, contentPadding, setContentPadding, 12 )

    // 时间轴连接线颜色；无效颜色表示使用 QPalette::Mid。
    EXWIDGETS_DECLARE_PROPERTY( QColor, lineColor, lineColor, setLineColor, QColor() )

    // 标题字号，0 表示使用控件字体。
    EXWIDGETS_DECLARE_PROPERTY( int, titleFontPixelSize, titleFontPixelSize, setTitleFontPixelSize, 0 )

    // 描述字号，0 表示根据控件字体自动计算。
    EXWIDGETS_DECLARE_PROPERTY( int, descriptionFontPixelSize, descriptionFontPixelSize, setDescriptionFontPixelSize, 0 )

    // 时间文字字号，0 表示根据控件字体自动计算。
    EXWIDGETS_DECLARE_PROPERTY( int, timestampFontPixelSize, timestampFontPixelSize, setTimestampFontPixelSize, 0 )

    // 是否为 Current 节点播放呼吸动画。
    EXWIDGETS_DECLARE_PROPERTY( bool, animationEnabled, isAnimationEnabled, setAnimationEnabled, true )

    // Current 节点完成一次呼吸动画的时长，单位为毫秒。
    EXWIDGETS_DECLARE_PROPERTY( int, animationDuration, animationDuration, setAnimationDuration, 1400 )

    explicit ExTimeline( QWidget* parent = nullptr );
    ~ExTimeline() override;

    [[nodiscard]] QList<ExTimelineEvent*> events() const;
    [[nodiscard]] ExTimelineEvent* eventAt( int visualIndex ) const;
    ExTimelineEvent* addEvent( const QDateTime& timestamp,
                               const QString& title,
                               const QString& description     = QString(),
                               ExTimelineEvent::Status status = ExTimelineEvent::Normal );
    void addEvent( ExTimelineEvent* event );
    ExTimelineEvent* takeEvent( ExTimelineEvent* event );
    void removeEvent( ExTimelineEvent* event );
    void clearEvents();

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

    Q_SIGNAL void eventsChanged();
    Q_SIGNAL void eventClicked( ExTimelineEvent* event );
    Q_SIGNAL void eventActivated( ExTimelineEvent* event );

protected:
    void showEvent( QShowEvent* event ) override;
    void hideEvent( QHideEvent* event ) override;
    void changeEvent( QEvent* event ) override;

private:
    friend class ExTimelineDelegate;

    [[nodiscard]] QString formattedTimestamp( const ExTimelineEvent* event ) const;
    [[nodiscard]] QColor resolvedEventColor( const ExTimelineEvent* event, QPalette::ColorGroup group ) const;
    [[nodiscard]] qreal pulseProgress() const;
    void applyViewOrientation();
    void connectEvent( ExTimelineEvent* event );
    void refreshItemLayout();
    void updateAnimationState();

    using QListView::setFlow;
    using QListView::setItemDelegate;
    using QListView::setModel;
    using QListView::setWrapping;

    ExTimelineModel* m_timelineModel       = nullptr;
    ExTimelineDelegate* m_timelineDelegate = nullptr;
    QVariantAnimation* m_pulseAnimation    = nullptr;
    qreal m_pulseProgress                  = 0.0;
};
