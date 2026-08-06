#include "timelineshowcasewidget.h"

#include "fluentui3styleproperties.h"

#include <excolorpickerbutton.h>
#include <excombobox.h>
#include <extabwidget.h>
#include <extimeline.h>

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDateTime>
#include <QFont>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabBar>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
QWidget* makeCard( QWidget* parent )
{
    auto* card = new QWidget( parent );
    card->setProperty( "isCard", true );
    card->setAttribute( Qt::WA_StyledBackground, true );
    return card;
}

QLabel* makeSectionTitle( const QString& text, QWidget* parent )
{
    auto* label = new QLabel( text, parent );
    QFont font = label->font();
    font.setBold( true );
    font.setPixelSize( 14 );
    label->setFont( font );
    return label;
}

QSlider* makeValueSlider( QWidget* parent,
                          int minimum,
                          int maximum,
                          int value,
                          int scale = 1,
                          int precision = 0 )
{
    auto* slider = new QSlider( Qt::Horizontal, parent );
    slider->setRange( minimum, maximum );
    slider->setValue( value );
    slider->setTracking( true );
    slider->setMinimumWidth( 180 );
    slider->setProperty( SliderValueTipProperty, true );
    if ( scale > 1 )
    {
        slider->setProperty( "scale", scale );
        slider->setProperty( "precision", precision );
    }
    return slider;
}

QColor fallbackEventColor( const ExTimeline* timeline, const ExTimelineEvent* event )
{
    if ( event && event->color().isValid() )
    {
        return event->color();
    }
    if ( event )
    {
        switch ( event->status() )
        {
        case ExTimelineEvent::Completed:
            return QColor( QStringLiteral( "#107C10" ) );
        case ExTimelineEvent::Warning:
            return QColor( QStringLiteral( "#F2A900" ) );
        case ExTimelineEvent::Error:
            return QColor( QStringLiteral( "#D13438" ) );
        case ExTimelineEvent::Pending:
            return timeline->palette().color( QPalette::Mid );
        default:
            break;
        }
    }
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return timeline->palette().color( QPalette::Accent );
#else
    return timeline->palette().color( QPalette::Highlight );
#endif
}

QWidget* makeTimelineSample( const QString& title,
                             const QString& subtitle,
                             ExTimeline* timeline,
                             QWidget* parent )
{
    auto* sample = makeCard( parent );
    auto* layout = new QVBoxLayout( sample );
    layout->setContentsMargins( 14, 14, 14, 14 );
    layout->setSpacing( 6 );
    layout->addWidget( makeSectionTitle( title, sample ) );
    auto* hint = new QLabel( subtitle, sample );
    hint->setProperty( "isSecondaryText", true );
    hint->setWordWrap( true );
    layout->addWidget( hint );
    timeline->setParent( sample );
    timeline->setMinimumWidth( 340 );
    timeline->setFixedHeight( 330 );
    layout->addWidget( timeline );
    return sample;
}
}

TimelineShowcaseWidget::TimelineShowcaseWidget( QWidget* parent )
    : QFrame( parent )
{
    setFrameShape( QFrame::StyledPanel );

    auto* rootLayout = new QVBoxLayout( this );
    rootLayout->setContentsMargins( 0, 0, 0, 0 );

    auto* scrollArea = new QScrollArea( this );
    scrollArea->setWidgetResizable( true );
    scrollArea->setFrameShape( QFrame::NoFrame );
    scrollArea->setAutoFillBackground( false );
    scrollArea->viewport()->setAutoFillBackground( false );
    rootLayout->addWidget( scrollArea );

    auto* content = new QWidget( scrollArea );
    content->setAutoFillBackground( false );
    auto* mainLayout = new QVBoxLayout( content );
    mainLayout->setContentsMargins( 16, 16, 16, 16 );
    mainLayout->setSpacing( 16 );

    auto* title = new QLabel( QStringLiteral( "ExTimeline" ), content );
    QFont titleFont = title->font();
    titleFont.setPointSize( 16 );
    titleFont.setBold( true );
    title->setFont( titleFont );
    mainLayout->addWidget( title );

    auto* description = new QLabel(
        tr( "用于展示事件与状态变化；支持水平/垂直、正序/倒序、单侧/交错布局和当前事件动画。" ),
        content );
    description->setWordWrap( true );
    mainLayout->addWidget( description );

    auto* examplesGrid = new QGridLayout;
    examplesGrid->setHorizontalSpacing( 16 );
    examplesGrid->setVerticalSpacing( 16 );
    for ( int column = 0; column < 3; ++column )
    {
        examplesGrid->setColumnStretch( column, 1 );
    }

    auto* orderTimeline = new ExTimeline;
    orderTimeline->setLayoutMode( ExTimeline::ContentOnRight );
    orderTimeline->setTimestampFormat( QStringLiteral( "HH:mm" ) );
    orderTimeline->addEvent( QDateTime::currentDateTime().addSecs( -5400 ),
                             tr( "订单已提交" ),
                             tr( "订单信息已进入处理队列。" ),
                             ExTimelineEvent::Completed );
    orderTimeline->addEvent( QDateTime::currentDateTime().addSecs( -3600 ),
                             tr( "付款成功" ),
                             tr( "支付信息已经确认。" ),
                             ExTimelineEvent::Completed );
    orderTimeline->addEvent( QDateTime::currentDateTime().addSecs( -900 ),
                             tr( "正在打包" ),
                             tr( "仓库正在准备商品。" ),
                             ExTimelineEvent::Current );
    orderTimeline->addEvent( QDateTime::currentDateTime().addSecs( 1800 ),
                             tr( "等待发货" ),
                             tr( "物流单号生成后将自动更新。" ),
                             ExTimelineEvent::Pending );
    examplesGrid->addWidget( makeTimelineSample( tr( "订单流程" ),
                                                  tr( "内容在右侧，完成、当前和等待状态使用不同节点。" ),
                                                  orderTimeline,
                                                  content ),
                             0,
                             0 );

    auto* updateTimeline = new ExTimeline;
    updateTimeline->setLayoutMode( ExTimeline::ContentOnRight );
    updateTimeline->setReverse( true );
    auto* updateReady = updateTimeline->addEvent( QDateTime(),
                                                  tr( "版本 2.4.0 已发布" ),
                                                  tr( "新增时间轴与多数据仪表盘。" ),
                                                  ExTimelineEvent::Completed );
    updateReady->setTimeText( tr( "3 天前" ) );
    auto* updateTesting = updateTimeline->addEvent( QDateTime(),
                                                    tr( "完成回归测试" ),
                                                    tr( "控件交互与主题切换检查通过。" ),
                                                    ExTimelineEvent::Completed );
    updateTesting->setTimeText( tr( "昨天" ) );
    auto* updateCurrent = updateTimeline->addEvent( QDateTime(),
                                                    tr( "准备发布说明" ),
                                                    tr( "正在整理新增 API 和示例。" ),
                                                    ExTimelineEvent::Current );
    updateCurrent->setTimeText( tr( "刚刚" ) );
    examplesGrid->addWidget( makeTimelineSample( tr( "更新记录" ),
                                                  tr( "倒序显示，事件可以使用相对时间文本。" ),
                                                  updateTimeline,
                                                  content ),
                             0,
                             1 );

    auto* systemTimeline = new ExTimeline;
    systemTimeline->setLayoutMode( ExTimeline::Alternating );
    systemTimeline->setTimestampFormat( QStringLiteral( "HH:mm:ss" ) );
    systemTimeline->addEvent( QDateTime::currentDateTime().addSecs( -300 ),
                              tr( "服务已启动" ),
                              tr( "监听端口 8080。" ),
                              ExTimelineEvent::Completed );
    systemTimeline->addEvent( QDateTime::currentDateTime().addSecs( -160 ),
                              tr( "内存占用偏高" ),
                              tr( "当前使用率为 78%。" ),
                              ExTimelineEvent::Warning );
    systemTimeline->addEvent( QDateTime::currentDateTime().addSecs( -70 ),
                              tr( "连接中断" ),
                              tr( "远程节点暂时不可用。" ),
                              ExTimelineEvent::Error );
    systemTimeline->addEvent( QDateTime::currentDateTime(),
                              tr( "正在重新连接" ),
                              tr( "将在几秒后再次尝试。" ),
                              ExTimelineEvent::Current );
    examplesGrid->addWidget( makeTimelineSample( tr( "系统事件" ),
                                                  tr( "交错布局适合同时展示状态、时间和较短的事件内容。" ),
                                                  systemTimeline,
                                                  content ),
                             0,
                             2 );

    auto* horizontalTimeline = new ExTimeline;
    horizontalTimeline->setOrientation( Qt::Horizontal );
    horizontalTimeline->setLayoutMode( ExTimeline::Alternating );
    horizontalTimeline->setHorizontalItemWidth( 210 );
    horizontalTimeline->setTimestampWidth( 88 );
    auto* createdEvent = horizontalTimeline->addEvent( QDateTime(), tr( "创建" ), tr( "提交任务" ), ExTimelineEvent::Completed );
    createdEvent->setTimeText( tr( "09:00" ) );
    auto* reviewedEvent = horizontalTimeline->addEvent( QDateTime(), tr( "审核" ), tr( "确认内容" ), ExTimelineEvent::Completed );
    reviewedEvent->setTimeText( tr( "09:20" ) );
    auto* processingEvent = horizontalTimeline->addEvent( QDateTime(), tr( "处理" ), tr( "正在执行" ), ExTimelineEvent::Current );
    processingEvent->setTimeText( tr( "09:35" ) );
    auto* deliveryEvent = horizontalTimeline->addEvent( QDateTime(), tr( "交付" ), tr( "等待完成" ), ExTimelineEvent::Pending );
    deliveryEvent->setTimeText( tr( "10:00" ) );
    QWidget* horizontalSample = makeTimelineSample( tr( "水平时间轴" ),
                                                     tr( "主线横向延伸，支持上方、下方和上下交错。" ),
                                                     horizontalTimeline,
                                                     content );
    horizontalTimeline->setFixedHeight( 250 );
    examplesGrid->addWidget( horizontalSample, 1, 0, 1, 3 );

    mainLayout->addLayout( examplesGrid );

    auto* propertiesCard = makeCard( content );
    auto* propertiesLayout = new QVBoxLayout( propertiesCard );
    propertiesLayout->setContentsMargins( 16, 16, 16, 16 );
    propertiesLayout->setSpacing( 12 );
    propertiesLayout->addWidget( makeSectionTitle( tr( "实时属性" ), propertiesCard ) );

    auto* propertyPreviewLayout = new QHBoxLayout;
    propertyPreviewLayout->setSpacing( 28 );

    auto* previewTimeline = new ExTimeline( propertiesCard );
    previewTimeline->setMinimumHeight( 470 );
    previewTimeline->setLayoutMode( ExTimeline::Alternating );
    previewTimeline->setTimestampFormat( QStringLiteral( "HH:mm" ) );
    previewTimeline->addEvent( QDateTime::currentDateTime().addSecs( -2400 ),
                               tr( "创建任务" ),
                               tr( "任务已添加到计划中。" ),
                               ExTimelineEvent::Completed );
    previewTimeline->addEvent( QDateTime::currentDateTime().addSecs( -1200 ),
                               tr( "下载资源" ),
                               tr( "所需资源已经准备完成。" ),
                               ExTimelineEvent::Completed );
    previewTimeline->addEvent( QDateTime::currentDateTime().addSecs( -300 ),
                               tr( "处理数据" ),
                               tr( "当前正在生成结果。" ),
                               ExTimelineEvent::Current );
    previewTimeline->addEvent( QDateTime::currentDateTime().addSecs( 900 ),
                               tr( "等待确认" ),
                               tr( "处理完成后需要人工确认。" ),
                               ExTimelineEvent::Pending );
    propertyPreviewLayout->addWidget( previewTimeline, 1 );

    auto* editorTabs = new ExTabWidget( propertiesCard );
    editorTabs->tabBar()->setProperty( TabBarStyleProperty, TabBarStyle::Pivot_Slide );
    editorTabs->setMinimumWidth( 440 );
    editorTabs->setMinimumHeight( 470 );

    auto* layoutPage = new QWidget( editorTabs );
    auto* layoutForm = new QFormLayout( layoutPage );
    layoutForm->setContentsMargins( 12, 12, 12, 12 );
    layoutForm->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    layoutForm->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
    layoutForm->setVerticalSpacing( 9 );

    auto* stylePage = new QWidget( editorTabs );
    auto* styleForm = new QFormLayout( stylePage );
    styleForm->setContentsMargins( 12, 12, 12, 12 );
    styleForm->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    styleForm->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
    styleForm->setVerticalSpacing( 9 );

    auto* eventPage = new QWidget( editorTabs );
    auto* eventForm = new QFormLayout( eventPage );
    eventForm->setContentsMargins( 12, 12, 12, 12 );
    eventForm->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    eventForm->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
    eventForm->setVerticalSpacing( 9 );

    editorTabs->addTab( layoutPage, tr( "布局" ) );
    editorTabs->addTab( stylePage, tr( "样式" ) );
    editorTabs->addTab( eventPage, tr( "事件" ) );

    auto* orientationCombo = new ExComboBox( layoutPage );
    orientationCombo->addItem( tr( "垂直" ), Qt::Vertical );
    orientationCombo->addItem( tr( "水平" ), Qt::Horizontal );
    orientationCombo->setCurrentIndex( orientationCombo->findData( previewTimeline->orientation() ) );
    auto* layoutModeCombo = new ExComboBox( layoutPage );
    layoutModeCombo->addItem( tr( "右侧 / 下方" ), ExTimeline::ContentOnRight );
    layoutModeCombo->addItem( tr( "左侧 / 上方" ), ExTimeline::ContentOnLeft );
    layoutModeCombo->addItem( tr( "左右 / 上下交错" ), ExTimeline::Alternating );
    layoutModeCombo->addItem( tr( "反向交错" ), ExTimeline::AlternatingReverse );
    layoutModeCombo->setCurrentIndex( layoutModeCombo->findData( previewTimeline->layoutMode() ) );
    auto* reverseCheck = new QCheckBox( tr( "反转显示顺序" ), layoutPage );
    reverseCheck->setChecked( previewTimeline->isReverse() );
    auto* timestampVisibleCheck = new QCheckBox( tr( "显示时间" ), layoutPage );
    timestampVisibleCheck->setChecked( previewTimeline->isTimestampVisible() );
    auto* descriptionVisibleCheck = new QCheckBox( tr( "显示描述" ), layoutPage );
    descriptionVisibleCheck->setChecked( previewTimeline->isDescriptionVisible() );
    auto* animationEnabledCheck = new QCheckBox( tr( "当前节点动画" ), stylePage );
    animationEnabledCheck->setChecked( previewTimeline->isAnimationEnabled() );
    auto* timestampFormatEdit = new QLineEdit( previewTimeline->timestampFormat(), layoutPage );
    auto* timestampWidthSlider = makeValueSlider( layoutPage, 40, 240, previewTimeline->timestampWidth() );
    auto* nodeSizeSlider = makeValueSlider( stylePage, 6, 40, previewTimeline->nodeSize() );
    auto* lineWidthSlider = makeValueSlider( stylePage,
                                             1,
                                             20,
                                             qRound( previewTimeline->lineWidth() * 2.0 ),
                                             2,
                                             1 );
    auto* itemSpacingSlider = makeValueSlider( layoutPage, 0, 80, previewTimeline->itemSpacing() );
    auto* horizontalItemWidthSlider = makeValueSlider( layoutPage,
                                                        120,
                                                        480,
                                                        previewTimeline->horizontalItemWidth() );
    horizontalItemWidthSlider->setEnabled( previewTimeline->orientation() == Qt::Horizontal );
    auto* contentPaddingSlider = makeValueSlider( layoutPage, 0, 80, previewTimeline->contentPadding() );
    auto* titleFontSizeSlider = makeValueSlider( stylePage, 0, 36, previewTimeline->titleFontPixelSize() );
    auto* descriptionFontSizeSlider = makeValueSlider( stylePage,
                                                        0,
                                                        36,
                                                        previewTimeline->descriptionFontPixelSize() );
    auto* timestampFontSizeSlider = makeValueSlider( stylePage,
                                                      0,
                                                      36,
                                                      previewTimeline->timestampFontPixelSize() );
    auto* animationDurationSlider = makeValueSlider( stylePage,
                                                      200,
                                                      4000,
                                                      previewTimeline->animationDuration() );
    auto* lineColorButton = new ExColorPickerButton( stylePage );
    lineColorButton->setSelectedColor( previewTimeline->palette().color( QPalette::Mid ) );

    layoutForm->addRow( tr( "方向" ), orientationCombo );
    layoutForm->addRow( tr( "布局模式" ), layoutModeCombo );
    layoutForm->addRow( reverseCheck );
    layoutForm->addRow( timestampVisibleCheck );
    layoutForm->addRow( descriptionVisibleCheck );
    layoutForm->addRow( tr( "时间格式" ), timestampFormatEdit );
    layoutForm->addRow( tr( "时间列宽" ), timestampWidthSlider );
    layoutForm->addRow( tr( "事件间距" ), itemSpacingSlider );
    layoutForm->addRow( tr( "水平项宽度" ), horizontalItemWidthSlider );
    layoutForm->addRow( tr( "左右边距" ), contentPaddingSlider );

    styleForm->addRow( animationEnabledCheck );
    styleForm->addRow( tr( "节点大小" ), nodeSizeSlider );
    styleForm->addRow( tr( "连接线宽度" ), lineWidthSlider );
    styleForm->addRow( tr( "标题字号（0 自动）" ), titleFontSizeSlider );
    styleForm->addRow( tr( "描述字号（0 自动）" ), descriptionFontSizeSlider );
    styleForm->addRow( tr( "时间字号（0 自动）" ), timestampFontSizeSlider );
    styleForm->addRow( tr( "动画时长" ), animationDurationSlider );
    styleForm->addRow( tr( "连接线颜色" ), lineColorButton );

    auto* eventSelector = new ExComboBox( eventPage );
    for ( const ExTimelineEvent* event : previewTimeline->events() )
    {
        eventSelector->addItem( event->title() );
    }
    auto* eventTimeEdit = new QLineEdit( eventPage );
    auto* eventTitleEdit = new QLineEdit( eventPage );
    auto* eventDescriptionEdit = new QLineEdit( eventPage );
    auto* eventStatusCombo = new ExComboBox( eventPage );
    eventStatusCombo->addItem( tr( "普通" ), ExTimelineEvent::Normal );
    eventStatusCombo->addItem( tr( "已完成" ), ExTimelineEvent::Completed );
    eventStatusCombo->addItem( tr( "当前" ), ExTimelineEvent::Current );
    eventStatusCombo->addItem( tr( "等待" ), ExTimelineEvent::Pending );
    eventStatusCombo->addItem( tr( "警告" ), ExTimelineEvent::Warning );
    eventStatusCombo->addItem( tr( "错误" ), ExTimelineEvent::Error );
    auto* eventPlacementCombo = new ExComboBox( eventPage );
    eventPlacementCombo->addItem( tr( "自动" ), ExTimelineEvent::Automatic );
    eventPlacementCombo->addItem( tr( "左侧 / 上方" ), ExTimelineEvent::LeftSide );
    eventPlacementCombo->addItem( tr( "右侧 / 下方" ), ExTimelineEvent::RightSide );
    auto* eventIconEdit = new QLineEdit( eventPage );
    eventIconEdit->setPlaceholderText( tr( "可选 Fluent 字体图标字符" ) );
    auto* eventColorButton = new ExColorPickerButton( eventPage );
    auto* addEventButton = new QPushButton( tr( "添加事件" ), eventPage );
    auto* removeEventButton = new QPushButton( tr( "删除当前事件" ), eventPage );
    auto* eventButtonLayout = new QHBoxLayout;
    eventButtonLayout->setContentsMargins( 0, 0, 0, 0 );
    eventButtonLayout->addWidget( addEventButton );
    eventButtonLayout->addWidget( removeEventButton );

    eventForm->addRow( tr( "当前事件" ), eventSelector );
    eventForm->addRow( tr( "时间文字" ), eventTimeEdit );
    eventForm->addRow( tr( "标题" ), eventTitleEdit );
    eventForm->addRow( tr( "描述" ), eventDescriptionEdit );
    eventForm->addRow( tr( "状态" ), eventStatusCombo );
    eventForm->addRow( tr( "交错内容位置" ), eventPlacementCombo );
    eventForm->addRow( tr( "字体图标" ), eventIconEdit );
    eventForm->addRow( tr( "节点颜色" ), eventColorButton );
    eventForm->addRow( eventButtonLayout );

    const auto currentEvent = [=]() -> ExTimelineEvent*
    {
        const int index = eventSelector->currentIndex();
        const QList<ExTimelineEvent*> events = previewTimeline->events();
        return index >= 0 && index < events.size() ? events.at( index ) : nullptr;
    };
    const auto loadEventEditor = [=]( int index )
    {
        const QList<ExTimelineEvent*> events = previewTimeline->events();
        ExTimelineEvent* event = index >= 0 && index < events.size() ? events.at( index ) : nullptr;
        const bool enabled = event != nullptr;
        eventTimeEdit->setEnabled( enabled );
        eventTitleEdit->setEnabled( enabled );
        eventDescriptionEdit->setEnabled( enabled );
        eventStatusCombo->setEnabled( enabled );
        eventPlacementCombo->setEnabled( enabled );
        eventIconEdit->setEnabled( enabled );
        eventColorButton->setEnabled( enabled );
        removeEventButton->setEnabled( enabled );
        if ( !event )
        {
            const QSignalBlocker timeBlocker( eventTimeEdit );
            const QSignalBlocker titleBlocker( eventTitleEdit );
            const QSignalBlocker descriptionBlocker( eventDescriptionEdit );
            const QSignalBlocker iconBlocker( eventIconEdit );
            eventTimeEdit->clear();
            eventTitleEdit->clear();
            eventDescriptionEdit->clear();
            eventIconEdit->clear();
            return;
        }
        const QSignalBlocker timeBlocker( eventTimeEdit );
        const QSignalBlocker titleBlocker( eventTitleEdit );
        const QSignalBlocker descriptionBlocker( eventDescriptionEdit );
        const QSignalBlocker statusBlocker( eventStatusCombo );
        const QSignalBlocker placementBlocker( eventPlacementCombo );
        const QSignalBlocker iconBlocker( eventIconEdit );
        const QSignalBlocker colorBlocker( eventColorButton );
        eventTimeEdit->setText( event->timeText().isEmpty()
                                    ? event->timestamp().toString( previewTimeline->timestampFormat() )
                                    : event->timeText() );
        eventTitleEdit->setText( event->title() );
        eventDescriptionEdit->setText( event->description() );
        eventStatusCombo->setCurrentIndex( eventStatusCombo->findData( event->status() ) );
        eventPlacementCombo->setCurrentIndex( eventPlacementCombo->findData( event->placement() ) );
        eventIconEdit->setText( event->icon() );
        eventColorButton->setSelectedColor( fallbackEventColor( previewTimeline, event ) );
    };

    connect( orientationCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             previewTimeline,
             [=]( int )
             {
                 const auto orientation = static_cast<Qt::Orientation>( orientationCombo->currentData().toInt() );
                 previewTimeline->setOrientation( orientation );
                 horizontalItemWidthSlider->setEnabled( orientation == Qt::Horizontal );
                 itemSpacingSlider->setEnabled( orientation == Qt::Vertical );
             } );
    connect( layoutModeCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             previewTimeline,
             [=]( int )
             {
                 previewTimeline->setLayoutMode(
                     static_cast<ExTimeline::LayoutMode>( layoutModeCombo->currentData().toInt() ) );
             } );
    connect( reverseCheck, &QCheckBox::toggled, previewTimeline, &ExTimeline::setReverse );
    connect( timestampVisibleCheck,
             &QCheckBox::toggled,
             previewTimeline,
             &ExTimeline::setTimestampVisible );
    connect( descriptionVisibleCheck,
             &QCheckBox::toggled,
             previewTimeline,
             &ExTimeline::setDescriptionVisible );
    connect( animationEnabledCheck,
             &QCheckBox::toggled,
             previewTimeline,
             &ExTimeline::setAnimationEnabled );
    connect( timestampFormatEdit,
             &QLineEdit::textChanged,
             previewTimeline,
             &ExTimeline::setTimestampFormat );
    connect( timestampWidthSlider, &QSlider::valueChanged, previewTimeline, &ExTimeline::setTimestampWidth );
    connect( nodeSizeSlider, &QSlider::valueChanged, previewTimeline, &ExTimeline::setNodeSize );
    connect( lineWidthSlider, &QSlider::valueChanged, previewTimeline, [=]( int value )
             {
                 previewTimeline->setLineWidth( value / 2.0 );
             } );
    connect( itemSpacingSlider, &QSlider::valueChanged, previewTimeline, &ExTimeline::setItemSpacing );
    connect( horizontalItemWidthSlider,
             &QSlider::valueChanged,
             previewTimeline,
             &ExTimeline::setHorizontalItemWidth );
    connect( contentPaddingSlider, &QSlider::valueChanged, previewTimeline, &ExTimeline::setContentPadding );
    connect( titleFontSizeSlider,
             &QSlider::valueChanged,
             previewTimeline,
             &ExTimeline::setTitleFontPixelSize );
    connect( descriptionFontSizeSlider,
             &QSlider::valueChanged,
             previewTimeline,
             &ExTimeline::setDescriptionFontPixelSize );
    connect( timestampFontSizeSlider,
             &QSlider::valueChanged,
             previewTimeline,
             &ExTimeline::setTimestampFontPixelSize );
    connect( animationDurationSlider,
             &QSlider::valueChanged,
             previewTimeline,
             &ExTimeline::setAnimationDuration );
    connect( lineColorButton,
             &ExColorPickerButton::selectedColorChanged,
             previewTimeline,
             &ExTimeline::setLineColor );

    connect( eventSelector,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             previewTimeline,
             loadEventEditor );
    connect( eventTimeEdit, &QLineEdit::textChanged, previewTimeline, [=]( const QString& text )
             {
                 if ( ExTimelineEvent* event = currentEvent() )
                 {
                     event->setTimeText( text );
                 }
             } );
    connect( eventTitleEdit, &QLineEdit::textChanged, previewTimeline, [=]( const QString& text )
             {
                 if ( ExTimelineEvent* event = currentEvent() )
                 {
                     event->setTitle( text );
                     const QSignalBlocker blocker( eventSelector );
                     eventSelector->setItemText( eventSelector->currentIndex(), text );
                 }
             } );
    connect( eventDescriptionEdit, &QLineEdit::textChanged, previewTimeline, [=]( const QString& text )
             {
                 if ( ExTimelineEvent* event = currentEvent() )
                 {
                     event->setDescription( text );
                 }
             } );
    connect( eventStatusCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             previewTimeline,
             [=]( int )
             {
                 if ( ExTimelineEvent* event = currentEvent() )
                 {
                     event->setStatus( static_cast<ExTimelineEvent::Status>(
                         eventStatusCombo->currentData().toInt() ) );
                 }
             } );
    connect( eventPlacementCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             previewTimeline,
             [=]( int )
             {
                 if ( ExTimelineEvent* event = currentEvent() )
                 {
                     event->setPlacement( static_cast<ExTimelineEvent::Placement>(
                         eventPlacementCombo->currentData().toInt() ) );
                 }
             } );
    connect( eventIconEdit, &QLineEdit::textChanged, previewTimeline, [=]( const QString& text )
             {
                 if ( ExTimelineEvent* event = currentEvent() )
                 {
                     event->setIcon( text );
                 }
             } );
    connect( eventColorButton,
             &ExColorPickerButton::selectedColorChanged,
             previewTimeline,
             [=]( const QColor& color )
             {
                 if ( ExTimelineEvent* event = currentEvent() )
                 {
                     event->setColor( color );
                 }
             } );
    connect( addEventButton, &QPushButton::clicked, previewTimeline, [=]
             {
                 ExTimelineEvent* event = previewTimeline->addEvent(
                     QDateTime::currentDateTime(),
                     tr( "新事件" ),
                     tr( "可以在右侧修改事件内容。" ),
                     ExTimelineEvent::Normal );
                 eventSelector->addItem( event->title() );
                 eventSelector->setCurrentIndex( eventSelector->count() - 1 );
             } );
    connect( removeEventButton, &QPushButton::clicked, previewTimeline, [=]
             {
                 const int index = eventSelector->currentIndex();
                 if ( ExTimelineEvent* event = currentEvent() )
                 {
                     previewTimeline->removeEvent( event );
                     eventSelector->removeItem( index );
                     loadEventEditor( eventSelector->currentIndex() );
                 }
             } );
    connect( previewTimeline, &ExTimeline::eventClicked, eventSelector, [=]( ExTimelineEvent* event )
             {
                 const int index = previewTimeline->events().indexOf( event );
                 if ( index >= 0 )
                 {
                     eventSelector->setCurrentIndex( index );
                 }
             } );

    loadEventEditor( 0 );
    propertyPreviewLayout->addWidget( editorTabs, 1 );
    propertiesLayout->addLayout( propertyPreviewLayout );

    auto* code = new QPlainTextEdit( propertiesCard );
    code->setReadOnly( true );
    code->setMaximumHeight( 170 );
    code->setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
    code->setPlainText( QStringLiteral(
        "auto *timeline = new ExTimeline(parent);\n"
        "timeline->setOrientation(Qt::Horizontal);\n"
        "timeline->setLayoutMode(ExTimeline::Alternating);\n"
        "timeline->setHorizontalItemWidth(220);\n"
        "auto *event = timeline->addEvent(QDateTime::currentDateTime(),\n"
        "    tr(\"任务已完成\"), tr(\"结果已经保存。\"),\n"
        "    ExTimelineEvent::Completed);\n"
        "event->setColor(QColor(\"#107C10\"));" ) );
    propertiesLayout->addWidget( code );
    mainLayout->addWidget( propertiesCard );
    mainLayout->addStretch();

    scrollArea->setWidget( content );
}
