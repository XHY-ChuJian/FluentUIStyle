#include "radialgaugeshowcasewidget.h"

#include "fluentui3styleproperties.h"

#include <excolorpickerbutton.h>
#include <excombobox.h>
#include <exradialgauge.h>
#include <extabwidget.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QVBoxLayout>
#include <QtMath>

#include <utility>

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

QPalette::ColorRole accentRole()
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return QPalette::Accent;
#else
    return QPalette::Highlight;
#endif
}

QColor gaugeAccentColor( const QWidget* widget )
{
    return widget->palette().color( QPalette::Active, accentRole() );
}

void setGaugeAccentColor( QWidget* widget, const QColor& color )
{
    QPalette palette = widget->palette();
    palette.setColor( QPalette::Active, accentRole(), color );
    palette.setColor( QPalette::Inactive, accentRole(), color );
    widget->setPalette( palette );
}

void setGaugeTrackColor( QWidget* widget, const QColor& color )
{
    QPalette palette = widget->palette();
    palette.setColor( QPalette::Active, QPalette::Mid, color );
    palette.setColor( QPalette::Inactive, QPalette::Mid, color );
    widget->setPalette( palette );
}

QSlider* makeValueSlider( QWidget* parent,
                          int minimum,
                          int maximum,
                          int value,
                          int singleStep = 1,
                          int pageStep = 10,
                          int scale = 1,
                          int precision = 0 )
{
    auto* slider = new QSlider( Qt::Horizontal, parent );
    slider->setRange( minimum, maximum );
    slider->setValue( value );
    slider->setSingleStep( singleStep );
    slider->setPageStep( pageStep );
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

template<typename Setter>
void connectScaledSlider( QSlider* slider, ExRadialGauge* gauge, qreal scale, Setter setter )
{
    QObject::connect( slider, &QSlider::valueChanged, gauge, [gauge, scale, setter]( int value )
                     {
                         ( gauge->*setter )( value / scale );
                     } );
}

void configureClassicGauge( ExRadialGauge* gauge )
{
    gauge->setRange( 0, 100 );
    gauge->setValue( 50 );
    gauge->setMinimumAngle( -140.0 );
    gauge->setMaximumAngle( 140.0 );
    gauge->setScaleMode( ExRadialGauge::TrackScale );
    gauge->setScaleWidth( 7.0 );
    gauge->setMajorTickCount( 11 );
    gauge->setMinorTickCount( 4 );
    gauge->setTickLength( 4.0 );
    gauge->setTickWidth( 1.0 );
    gauge->setMajorTickLength( 8.0 );
    gauge->setMajorTickWidth( 1.6 );
    gauge->setTickPadding( 9.0 );
    gauge->setLabelsVisible( true );
    gauge->setLabelPadding( 22.0 );
    gauge->setLabelFontPixelSize( 10 );
    gauge->setNeedleStyle( ExRadialGauge::TriangleNeedle );
    gauge->setNeedleWidth( 11.0 );
    gauge->setNeedleLength( 0.62 );
    gauge->setNeedleColor( QColor( QStringLiteral( "#EC1460" ) ) );
    gauge->setHubVisible( false );
    gauge->setTitle( QStringLiteral( "SCORE" ) );
    gauge->setValuePosition( ExRadialGauge::CenterValue );
    gauge->setValueFontPixelSize( 28 );
    gauge->setInteractive( true );

    QColor track = gauge->palette().color( QPalette::Text );
    track.setAlpha( 220 );
    setGaugeTrackColor( gauge, track );
}

void configureProgressGauge( ExRadialGauge* gauge )
{
    const QColor purple( QStringLiteral( "#8067FF" ) );
    gauge->setRange( 0, 100 );
    gauge->setValue( 70 );
    gauge->setMinimumAngle( -135.0 );
    gauge->setMaximumAngle( 135.0 );
    gauge->setScaleMode( ExRadialGauge::ProgressScale );
    gauge->setScaleWidth( 7.0 );
    gauge->setMajorTickCount( 11 );
    gauge->setMinorTickCount( 4 );
    gauge->setTickLength( 3.0 );
    gauge->setTickWidth( 1.0 );
    gauge->setMajorTickLength( 6.0 );
    gauge->setMajorTickWidth( 1.5 );
    gauge->setTickPadding( 12.0 );
    gauge->setLabelsVisible( true );
    gauge->setLabelPadding( 23.0 );
    gauge->setLabelFontPixelSize( 10 );
    gauge->setNeedleStyle( ExRadialGauge::TriangleNeedle );
    gauge->setNeedleWidth( 11.0 );
    gauge->setNeedleLength( 0.66 );
    gauge->setNeedleColor( purple );
    gauge->setHubVisible( true );
    gauge->setHubRadius( 9.0 );
    gauge->setValuePosition( ExRadialGauge::BottomValue );
    gauge->setValueFontPixelSize( 40 );
    gauge->setValueColor( purple );
    gauge->setInteractive( true );

    setGaugeAccentColor( gauge, purple );
    QColor track = gauge->palette().color( QPalette::Text );
    track.setAlpha( 210 );
    setGaugeTrackColor( gauge, track );
}

void configureSpeedometerGauge( ExRadialGauge* gauge )
{
    const QColor cyan( QStringLiteral( "#21BCE2" ) );
    const QColor amber( QStringLiteral( "#FFB900" ) );
    const QColor coral( QStringLiteral( "#FF6475" ) );
    gauge->setRange( 0, 100 );
    gauge->setValue( 70 );
    gauge->setMinimumAngle( -135.0 );
    gauge->setMaximumAngle( 135.0 );
    gauge->setScaleMode( ExRadialGauge::RangeScale );
    gauge->setScaleWidth( 7.0 );
    gauge->clearRanges();
    gauge->addRange( 0, 60, cyan );
    gauge->addRange( 60, 80, amber );
    gauge->addRange( 80, 100, coral );
    gauge->setMajorTickCount( 11 );
    gauge->setMinorTickCount( 4 );
    gauge->setTickLength( 4.0 );
    gauge->setTickWidth( 1.0 );
    gauge->setMajorTickLength( 8.0 );
    gauge->setMajorTickWidth( 1.6 );
    gauge->setTickPadding( 10.0 );
    gauge->setTickColor( QColor( 245, 247, 250, 220 ) );
    gauge->setLabelsVisible( true );
    gauge->setLabelPadding( 27.0 );
    gauge->setLabelFontPixelSize( 10 );
    gauge->setNeedleStyle( ExRadialGauge::TriangleNeedle );
    gauge->setNeedleWidth( 12.0 );
    gauge->setNeedleLength( 0.58 );
    gauge->setNeedleColor( cyan );
    gauge->setHubVisible( false );
    gauge->setUnit( QStringLiteral( "km/h" ) );
    gauge->setValuePosition( ExRadialGauge::BottomValue );
    gauge->setValueFontPixelSize( 24 );
    gauge->setValueColor( cyan );
    gauge->setInteractive( true );
}
}

RadialGaugeShowcaseWidget::RadialGaugeShowcaseWidget( QWidget* parent )
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

    auto* title = new QLabel( tr( "ExRadialGauge" ), content );
    QFont titleFont = title->font();
    titleFont.setPointSize( 16 );
    titleFont.setBold( true );
    title->setFont( titleFont );
    mainLayout->addWidget( title );

    auto* description = new QLabel(
        tr( "基于 QDial 的径向仪表盘，保留范围、数值和交互能力；同一控件可组合 Track、Progress、Ranges 刻度环、数字标签和不同指针样式。" ),
        content );
    description->setWordWrap( true );
    mainLayout->addWidget( description );

    auto* structuresCard = makeCard( content );
    auto* structuresLayout = new QVBoxLayout( structuresCard );
    structuresLayout->setContentsMargins( 16, 16, 16, 16 );
    structuresLayout->setSpacing( 12 );
    structuresLayout->addWidget( makeSectionTitle( tr( "同一控件的三种配置" ), structuresCard ) );

    auto* samplesLayout = new QHBoxLayout;
    samplesLayout->setSpacing( 20 );
    QList<ExRadialGauge*> sampleGauges;
    const auto addSample = [samplesLayout, structuresCard, &sampleGauges]( const QString& name,
                                                                         const auto& configure )
    {
        auto* sample = new QWidget( structuresCard );
        auto* sampleLayout = new QVBoxLayout( sample );
        sampleLayout->setContentsMargins( 0, 0, 0, 0 );
        sampleLayout->setSpacing( 6 );

        auto* gauge = new ExRadialGauge( sample );
        gauge->setFixedSize( 220, 220 );
        configure( gauge );

        auto* label = new QLabel( name, sample );
        label->setAlignment( Qt::AlignCenter );
        sampleLayout->addWidget( gauge, 0, Qt::AlignHCenter );
        sampleLayout->addWidget( label );
        samplesLayout->addWidget( sample, 1 );
        sampleGauges.append( gauge );
    };

    addSample( tr( "经典指针" ), configureClassicGauge );
    addSample( tr( "进度指针" ), configureProgressGauge );
    addSample( tr( "彩色区间" ), configureSpeedometerGauge );
    structuresLayout->addLayout( samplesLayout );

    auto* sharedValueLayout = new QHBoxLayout;
    auto* sharedValueLabel = new QLabel( tr( "公共数值" ), structuresCard );
    auto* sharedValueSlider = makeValueSlider( structuresCard, 0, 100, 70 );
    sharedValueLayout->addWidget( sharedValueLabel );
    sharedValueLayout->addWidget( sharedValueSlider, 1 );
    structuresLayout->addLayout( sharedValueLayout );

    QList<int> sampleAnimationDurations;
    sampleAnimationDurations.reserve( sampleGauges.size() );
    for ( ExRadialGauge* sampleGauge : std::as_const( sampleGauges ) )
    {
        sampleAnimationDurations.append( sampleGauge->valueAnimationDuration() );
    }

    for ( ExRadialGauge* sampleGauge : std::as_const( sampleGauges ) )
    {
        sampleGauge->setValue( sharedValueSlider->value() );
        connect( sampleGauge,
                 &QDial::sliderPressed,
                 structuresCard,
                 [sampleGauges]
                 {
                     for ( ExRadialGauge* gauge : sampleGauges )
                     {
                         gauge->setValueAnimationDuration( 0 );
                     }
                 } );
        connect( sampleGauge,
                 &QDial::sliderReleased,
                 structuresCard,
                 [sampleGauges, sampleAnimationDurations]
                 {
                     for ( qsizetype index = 0; index < sampleGauges.size(); ++index )
                     {
                         sampleGauges.at( index )->setValueAnimationDuration(
                             sampleAnimationDurations.at( index ) );
                     }
                 } );
        connect( sharedValueSlider,
                 &QSlider::valueChanged,
                 sampleGauge,
                 &ExRadialGauge::setValue );
        connect( sampleGauge,
                 &QDial::valueChanged,
                 sharedValueSlider,
                 [sampleGauge, sampleGauges, sharedValueSlider]( int value )
                 {
                     if ( sharedValueSlider->isSliderDown() || sampleGauge->isValueAnimating() )
                     {
                         return;
                     }

                     const QSignalBlocker blocker( sharedValueSlider );
                     sharedValueSlider->setValue( value );
                     for ( ExRadialGauge* otherGauge : sampleGauges )
                     {
                         if ( otherGauge != sampleGauge )
                         {
                             if ( sampleGauge->isSliderDown() )
                             {
                                 const QSignalBlocker otherGaugeBlocker( otherGauge );
                                 otherGauge->setValue( value );
                             }
                             else
                             {
                                 otherGauge->setValue( value );
                             }
                         }
                     }
                 } );
    }
    mainLayout->addWidget( structuresCard );

    auto* propertiesCard = makeCard( content );
    auto* propertiesLayout = new QVBoxLayout( propertiesCard );
    propertiesLayout->setContentsMargins( 16, 16, 16, 16 );
    propertiesLayout->setSpacing( 12 );
    propertiesLayout->addWidget( makeSectionTitle( tr( "实时属性" ), propertiesCard ) );

    auto* previewLayout = new QHBoxLayout;
    previewLayout->setSpacing( 32 );

    auto* gauge = new ExRadialGauge( propertiesCard );
    gauge->setObjectName( QStringLiteral( "radialGaugePreview" ) );
    gauge->setRange( 0, 240 );
    gauge->setValue( 210 );
    gauge->setSingleStep( 1 );
    gauge->setPageStep( 10 );
    gauge->setFixedSize( 300, 300 );
    const QColor firstRangeDefaultColor( QStringLiteral( "#21BCE2" ) );
    const QColor secondRangeDefaultColor( QStringLiteral( "#FFB900" ) );
    const QColor thirdRangeDefaultColor( QStringLiteral( "#FF6475" ) );
    auto* firstRange = gauge->addRange( 0, 80, firstRangeDefaultColor );
    auto* secondRange = gauge->addRange( 80, 160, secondRangeDefaultColor );
    auto* thirdRange = gauge->addRange( 160, 240, thirdRangeDefaultColor );
    previewLayout->addWidget( gauge, 0, Qt::AlignHCenter | Qt::AlignTop );

    auto* propertyTabs = new ExTabWidget( propertiesCard );
    propertyTabs->tabBar()->setProperty(TabBarStyleProperty, TabBarStyle::Pivot_Slide);
    propertyTabs->setMinimumWidth( 440 );
    propertyTabs->setMinimumHeight( 430 );

    const auto makePropertyForm = [propertyTabs]( QFormLayout*& form )
    {
        auto* page = new QWidget( propertyTabs );
        form = new QFormLayout( page );
        form->setContentsMargins( 12, 12, 12, 12 );
        form->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
        form->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
        form->setHorizontalSpacing( 12 );
        form->setVerticalSpacing( 10 );
        return page;
    };

    QFormLayout* basicForm = nullptr;
    QFormLayout* scaleForm = nullptr;
    QFormLayout* needleForm = nullptr;
    QFormLayout* rangeForm = nullptr;
    auto* basicPage = makePropertyForm( basicForm );
    auto* scalePage = makePropertyForm( scaleForm );
    auto* needlePage = makePropertyForm( needleForm );
    auto* rangePage = makePropertyForm( rangeForm );
    propertyTabs->addTab( basicPage, tr( "基础" ) );
    propertyTabs->addTab( scalePage, tr( "刻度与标签" ) );
    propertyTabs->addTab( needlePage, tr( "指针与文本" ) );
    propertyTabs->addTab( rangePage, tr( "彩色区间" ) );

    auto* scaleModeCombo = new ExComboBox( propertiesCard );
    scaleModeCombo->addItem( tr( "Track（纯轨道）" ), ExRadialGauge::TrackScale );
    scaleModeCombo->addItem( tr( "Progress（数值进度）" ), ExRadialGauge::ProgressScale );
    scaleModeCombo->addItem( tr( "Ranges（彩色区间）" ), ExRadialGauge::RangeScale );
    scaleModeCombo->setCurrentIndex( scaleModeCombo->findData( gauge->scaleMode() ) );

    const auto makeCapStyleCombo = [propertiesCard]( Qt::PenCapStyle currentStyle )
    {
        auto* combo = new ExComboBox( propertiesCard );
        combo->addItem( RadialGaugeShowcaseWidget::tr( "FlatCap" ), Qt::FlatCap );
        combo->addItem( RadialGaugeShowcaseWidget::tr( "SquareCap" ), Qt::SquareCap );
        combo->addItem( RadialGaugeShowcaseWidget::tr( "RoundCap" ), Qt::RoundCap );
        combo->setCurrentIndex( combo->findData( currentStyle ) );
        return combo;
    };
    auto* trackCapStyleCombo = makeCapStyleCombo( gauge->trackCapStyle() );
    auto* ringCapStyleCombo = makeCapStyleCombo( gauge->ringCapStyle() );

    auto* valueSlider = makeValueSlider( propertiesCard, gauge->minimum(), gauge->maximum(), gauge->value() );
    auto* valueAnimationDurationSlider = makeValueSlider( propertiesCard,
                                                          0,
                                                          1000,
                                                          gauge->valueAnimationDuration(),
                                                          10,
                                                          100 );

    auto* interactiveCheck = new QCheckBox( tr( "允许鼠标、键盘和滚轮交互" ), propertiesCard );
    interactiveCheck->setChecked( gauge->isInteractive() );

    auto* valueVisibleCheck = new QCheckBox( tr( "显示数值" ), propertiesCard );
    valueVisibleCheck->setChecked( gauge->isValueVisible() );

    auto* valuePositionCombo = new ExComboBox( propertiesCard );
    valuePositionCombo->addItem( tr( "中心" ), ExRadialGauge::CenterValue );
    valuePositionCombo->addItem( tr( "底部" ), ExRadialGauge::BottomValue );
    valuePositionCombo->setCurrentIndex( valuePositionCombo->findData( gauge->valuePosition() ) );

    auto* titleEdit = new QLineEdit( gauge->title(), propertiesCard );
    auto* unitEdit = new QLineEdit( gauge->unit(), propertiesCard );

    auto* labelsVisibleCheck = new QCheckBox( tr( "显示刻度数值" ), propertiesCard );
    labelsVisibleCheck->setChecked( gauge->areLabelsVisible() );

    auto* hubVisibleCheck = new QCheckBox( tr( "显示指针轴心" ), propertiesCard );
    hubVisibleCheck->setChecked( gauge->isHubVisible() );

    auto* needleStyleCombo = new ExComboBox( propertiesCard );
    needleStyleCombo->addItem( tr( "无指针" ), ExRadialGauge::NoNeedle );
    needleStyleCombo->addItem( tr( "线形指针" ), ExRadialGauge::LineNeedle );
    needleStyleCombo->addItem( tr( "三角指针" ), ExRadialGauge::TriangleNeedle );
    needleStyleCombo->setCurrentIndex( needleStyleCombo->findData( gauge->needleStyle() ) );

    auto* majorTickCountSlider = makeValueSlider( propertiesCard, 2, 100, gauge->majorTickCount() );
    auto* minorTickCountSlider = makeValueSlider( propertiesCard, 0, 20, gauge->minorTickCount() );

    auto makeMetricSlider = [propertiesCard]( qreal value, qreal minimum, qreal maximum )
    {
        constexpr int scale = 2;
        return makeValueSlider( propertiesCard,
                                qRound( minimum * scale ),
                                qRound( maximum * scale ),
                                qRound( value * scale ),
                                1,
                                4,
                                scale,
                                1 );
    };

    auto* scaleWidthSlider = makeMetricSlider( gauge->scaleWidth(), 0.5, 32.0 );
    auto* minimumAngleSlider = makeValueSlider( propertiesCard, -360, 360, qRound( gauge->minimumAngle() ), 5, 15 );
    auto* maximumAngleSlider = makeValueSlider( propertiesCard, -360, 360, qRound( gauge->maximumAngle() ), 5, 15 );
    auto* needleWidthSlider = makeMetricSlider( gauge->needleWidth(), 0.5, 24.0 );
    auto* needleLengthSlider = makeValueSlider( propertiesCard,
                                                5,
                                                100,
                                                qRound( gauge->needleLength() * 100.0 ),
                                                1,
                                                5,
                                                100,
                                                2 );
    auto* tickLengthSlider = makeMetricSlider( gauge->tickLength(), 0.0, 40.0 );
    auto* tickWidthSlider = makeMetricSlider( gauge->tickWidth(), 0.5, 16.0 );
    auto* majorTickLengthSlider = makeMetricSlider( gauge->majorTickLength(), 0.0, 40.0 );
    auto* majorTickWidthSlider = makeMetricSlider( gauge->majorTickWidth(), 0.5, 16.0 );
    auto* scalePaddingSlider = makeMetricSlider( gauge->scalePadding(), 0.0, 80.0 );
    auto* tickPaddingSlider = makeMetricSlider( gauge->tickPadding(), 0.0, 60.0 );
    auto* labelPaddingSlider = makeMetricSlider( gauge->labelPadding(), 0.0, 80.0 );
    auto* labelFontPixelSizeSlider = makeValueSlider( propertiesCard, 6, 48, gauge->labelFontPixelSize() );
    auto* hubRadiusSlider = makeMetricSlider( gauge->hubRadius(), 0.5, 30.0 );
    auto* valueFontPixelSizeSlider = makeValueSlider( propertiesCard, 0, 72, gauge->valueFontPixelSize() );

    auto* firstRangeFromSlider = makeValueSlider( propertiesCard,
                                                  gauge->minimum(),
                                                  gauge->maximum(),
                                                  firstRange->fromValue() );
    auto* firstRangeToSlider = makeValueSlider( propertiesCard,
                                                gauge->minimum(),
                                                gauge->maximum(),
                                                firstRange->toValue() );
    auto* secondRangeFromSlider = makeValueSlider( propertiesCard,
                                                   gauge->minimum(),
                                                   gauge->maximum(),
                                                   secondRange->fromValue() );
    auto* secondRangeToSlider = makeValueSlider( propertiesCard,
                                                 gauge->minimum(),
                                                 gauge->maximum(),
                                                 secondRange->toValue() );
    auto* thirdRangeFromSlider = makeValueSlider( propertiesCard,
                                                  gauge->minimum(),
                                                  gauge->maximum(),
                                                  thirdRange->fromValue() );
    auto* thirdRangeToSlider = makeValueSlider( propertiesCard,
                                                gauge->minimum(),
                                                gauge->maximum(),
                                                thirdRange->toValue() );

    auto* accentColorButton = new ExColorPickerButton( propertiesCard );
    accentColorButton->setSelectedColor( gaugeAccentColor( gauge ) );
    auto* trackColorButton = new ExColorPickerButton( propertiesCard );
    trackColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Mid ) );
    auto* needleColorButton = new ExColorPickerButton( propertiesCard );
    needleColorButton->setSelectedColor( gaugeAccentColor( gauge ) );
    auto* tickColorButton = new ExColorPickerButton( propertiesCard );
    tickColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
    auto* labelColorButton = new ExColorPickerButton( propertiesCard );
    labelColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
    auto* valueColorButton = new ExColorPickerButton( propertiesCard );
    valueColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
    auto* firstRangeColorButton = new ExColorPickerButton( propertiesCard );
    firstRangeColorButton->setSelectedColor( firstRange->color() );
    auto* secondRangeColorButton = new ExColorPickerButton( propertiesCard );
    secondRangeColorButton->setSelectedColor( secondRange->color() );
    auto* thirdRangeColorButton = new ExColorPickerButton( propertiesCard );
    thirdRangeColorButton->setSelectedColor( thirdRange->color() );

    auto* disabledCheck = new QCheckBox( tr( "禁用状态" ), propertiesCard );
    auto* resetButton = new QPushButton( tr( "恢复默认属性" ), propertiesCard );

    basicForm->addRow( tr( "Scale 模式" ), scaleModeCombo );
    basicForm->addRow( tr( "数值" ), valueSlider );
    basicForm->addRow( tr( "数值动画时长" ), valueAnimationDurationSlider );
    basicForm->addRow( interactiveCheck );
    basicForm->addRow( disabledCheck );
    basicForm->addRow( tr( "刻度环宽度" ), scaleWidthSlider );
    basicForm->addRow( tr( "起始角度" ), minimumAngleSlider );
    basicForm->addRow( tr( "结束角度" ), maximumAngleSlider );
    basicForm->addRow( tr( "外圈边距" ), scalePaddingSlider );
    basicForm->addRow( tr( "强调色" ), accentColorButton );
    basicForm->addRow( tr( "Track 颜色" ), trackColorButton );
    basicForm->addRow( resetButton );

    scaleForm->addRow( tr( "主刻度数量" ), majorTickCountSlider );
    scaleForm->addRow( tr( "每段次刻度数量" ), minorTickCountSlider );
    scaleForm->addRow( tr( "次刻度长度" ), tickLengthSlider );
    scaleForm->addRow( tr( "次刻度宽度" ), tickWidthSlider );
    scaleForm->addRow( tr( "主刻度长度" ), majorTickLengthSlider );
    scaleForm->addRow( tr( "主刻度宽度" ), majorTickWidthSlider );
    scaleForm->addRow( tr( "刻度边距" ), tickPaddingSlider );
    scaleForm->addRow( tr( "刻线颜色" ), tickColorButton );
    scaleForm->addRow( labelsVisibleCheck );
    scaleForm->addRow( tr( "标签边距" ), labelPaddingSlider );
    scaleForm->addRow( tr( "标签字号" ), labelFontPixelSizeSlider );
    scaleForm->addRow( tr( "标签颜色" ), labelColorButton );
    scaleForm->addRow( tr( "Track 端点" ), trackCapStyleCombo );
    scaleForm->addRow( tr( "环端点" ), ringCapStyleCombo );

    needleForm->addRow( tr( "指针样式" ), needleStyleCombo );
    needleForm->addRow( tr( "指针宽度" ), needleWidthSlider );
    needleForm->addRow( tr( "指针长度比例" ), needleLengthSlider );
    needleForm->addRow( tr( "指针颜色" ), needleColorButton );
    needleForm->addRow( hubVisibleCheck );
    needleForm->addRow( tr( "轴心半径" ), hubRadiusSlider );
    needleForm->addRow( valueVisibleCheck );
    needleForm->addRow( tr( "数值位置" ), valuePositionCombo );
    needleForm->addRow( tr( "标题" ), titleEdit );
    needleForm->addRow( tr( "单位" ), unitEdit );
    needleForm->addRow( tr( "数值字号" ), valueFontPixelSizeSlider );
    needleForm->addRow( tr( "数值颜色" ), valueColorButton );

    rangeForm->addRow( tr( "区间 1 起点" ), firstRangeFromSlider );
    rangeForm->addRow( tr( "区间 1 终点" ), firstRangeToSlider );
    rangeForm->addRow( tr( "区间 1 颜色" ), firstRangeColorButton );
    rangeForm->addRow( tr( "区间 2 起点" ), secondRangeFromSlider );
    rangeForm->addRow( tr( "区间 2 终点" ), secondRangeToSlider );
    rangeForm->addRow( tr( "区间 2 颜色" ), secondRangeColorButton );
    rangeForm->addRow( tr( "区间 3 起点" ), thirdRangeFromSlider );
    rangeForm->addRow( tr( "区间 3 终点" ), thirdRangeToSlider );
    rangeForm->addRow( tr( "区间 3 颜色" ), thirdRangeColorButton );

    previewLayout->addWidget( propertyTabs, 1 );
    propertiesLayout->addLayout( previewLayout );

    auto* angleHint = new QLabel(
        tr( "角度以正上方为 0°，顺时针为正；起止角度相同表示完整的 360°。" ),
        propertiesCard );
    angleHint->setWordWrap( true );
    propertiesLayout->addWidget( angleHint );

    const auto updateRangeEditorState = [=]
    {
        const bool enabled = gauge->scaleMode() == ExRadialGauge::RangeScale;
        firstRangeFromSlider->setEnabled( enabled );
        firstRangeToSlider->setEnabled( enabled );
        firstRangeColorButton->setEnabled( enabled );
        secondRangeFromSlider->setEnabled( enabled );
        secondRangeToSlider->setEnabled( enabled );
        secondRangeColorButton->setEnabled( enabled );
        thirdRangeFromSlider->setEnabled( enabled );
        thirdRangeToSlider->setEnabled( enabled );
        thirdRangeColorButton->setEnabled( enabled );
    };
    const auto updateLabelEditorState = [=]
    {
        const bool enabled = labelsVisibleCheck->isChecked();
        labelPaddingSlider->setEnabled( enabled );
        labelFontPixelSizeSlider->setEnabled( enabled );
        labelColorButton->setEnabled( enabled );
    };
    const auto updateNeedleEditorState = [=]
    {
        const auto style = static_cast<ExRadialGauge::NeedleStyle>( needleStyleCombo->currentData().toInt() );
        const bool needleEnabled = style != ExRadialGauge::NoNeedle;
        needleWidthSlider->setEnabled( needleEnabled );
        needleLengthSlider->setEnabled( needleEnabled );
        needleColorButton->setEnabled( needleEnabled );
        hubVisibleCheck->setEnabled( needleEnabled );
        hubRadiusSlider->setEnabled( needleEnabled && hubVisibleCheck->isChecked() );
    };
    const auto updateValueEditorState = [=]
    {
        const bool enabled = valueVisibleCheck->isChecked();
        valuePositionCombo->setEnabled( enabled );
        titleEdit->setEnabled( enabled );
        unitEdit->setEnabled( enabled );
        valueFontPixelSizeSlider->setEnabled( enabled );
        valueColorButton->setEnabled( enabled );
    };

    connect( scaleModeCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             gauge,
             [=]( int index )
             {
                 gauge->setScaleMode(
                     static_cast<ExRadialGauge::ScaleMode>( scaleModeCombo->itemData( index ).toInt() ) );
                 updateRangeEditorState();
             } );
    connect( valueSlider, &QSlider::valueChanged, gauge, &ExRadialGauge::setValue );
    connect( gauge, &QDial::valueChanged, valueSlider, [gauge, valueSlider]( int value )
             {
                 if ( valueSlider->isSliderDown() || gauge->isValueAnimating() )
                 {
                     return;
                 }
                 const QSignalBlocker blocker( valueSlider );
                 valueSlider->setValue( value );
             } );
    connect( valueAnimationDurationSlider,
             &QSlider::valueChanged,
             gauge,
             &ExRadialGauge::setValueAnimationDuration );
    connect( interactiveCheck, &QCheckBox::toggled, gauge, &ExRadialGauge::setInteractive );
    connect( valueVisibleCheck, &QCheckBox::toggled, gauge, &ExRadialGauge::setValueVisible );
    connect( valueVisibleCheck, &QCheckBox::toggled, gauge, [=]
             {
                 updateValueEditorState();
             } );
    connect( valuePositionCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             gauge,
             [=]( int index )
             {
                 gauge->setValuePosition(
                     static_cast<ExRadialGauge::ValuePosition>( valuePositionCombo->itemData( index ).toInt() ) );
             } );
    connect( titleEdit, &QLineEdit::textChanged, gauge, &ExRadialGauge::setTitle );
    connect( unitEdit, &QLineEdit::textChanged, gauge, &ExRadialGauge::setUnit );
    connect( valueFontPixelSizeSlider,
             &QSlider::valueChanged,
             gauge,
             &ExRadialGauge::setValueFontPixelSize );
    connect( majorTickCountSlider,
             &QSlider::valueChanged,
             gauge,
             &ExRadialGauge::setMajorTickCount );
    connect( minorTickCountSlider,
             &QSlider::valueChanged,
             gauge,
             &ExRadialGauge::setMinorTickCount );
    connectScaledSlider( scaleWidthSlider, gauge, 2.0, &ExRadialGauge::setScaleWidth );
    connectScaledSlider( minimumAngleSlider, gauge, 1.0, &ExRadialGauge::setMinimumAngle );
    connectScaledSlider( maximumAngleSlider, gauge, 1.0, &ExRadialGauge::setMaximumAngle );
    connectScaledSlider( needleWidthSlider, gauge, 2.0, &ExRadialGauge::setNeedleWidth );
    connectScaledSlider( needleLengthSlider, gauge, 100.0, &ExRadialGauge::setNeedleLength );
    connectScaledSlider( tickLengthSlider, gauge, 2.0, &ExRadialGauge::setTickLength );
    connectScaledSlider( tickWidthSlider, gauge, 2.0, &ExRadialGauge::setTickWidth );
    connectScaledSlider( majorTickLengthSlider, gauge, 2.0, &ExRadialGauge::setMajorTickLength );
    connectScaledSlider( majorTickWidthSlider, gauge, 2.0, &ExRadialGauge::setMajorTickWidth );
    connectScaledSlider( scalePaddingSlider, gauge, 2.0, &ExRadialGauge::setScalePadding );
    connectScaledSlider( tickPaddingSlider, gauge, 2.0, &ExRadialGauge::setTickPadding );
    connect( trackCapStyleCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             gauge,
             [=]( int index )
             {
                 gauge->setTrackCapStyle(
                     static_cast<Qt::PenCapStyle>( trackCapStyleCombo->itemData( index ).toInt() ) );
             } );
    connect( ringCapStyleCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             gauge,
             [=]( int index )
             {
                 gauge->setRingCapStyle(
                     static_cast<Qt::PenCapStyle>( ringCapStyleCombo->itemData( index ).toInt() ) );
             } );
    connect( labelsVisibleCheck, &QCheckBox::toggled, gauge, &ExRadialGauge::setLabelsVisible );
    connect( labelsVisibleCheck, &QCheckBox::toggled, gauge, [=]
             {
                 updateLabelEditorState();
             } );
    connectScaledSlider( labelPaddingSlider, gauge, 2.0, &ExRadialGauge::setLabelPadding );
    connect( labelFontPixelSizeSlider,
             &QSlider::valueChanged,
             gauge,
             &ExRadialGauge::setLabelFontPixelSize );
    connect( needleStyleCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             gauge,
             [=]( int index )
             {
                 gauge->setNeedleStyle(
                     static_cast<ExRadialGauge::NeedleStyle>( needleStyleCombo->itemData( index ).toInt() ) );
                 updateNeedleEditorState();
             } );
    connect( hubVisibleCheck, &QCheckBox::toggled, gauge, &ExRadialGauge::setHubVisible );
    connect( hubVisibleCheck, &QCheckBox::toggled, gauge, [=]
             {
                 updateNeedleEditorState();
             } );
    connectScaledSlider( hubRadiusSlider, gauge, 2.0, &ExRadialGauge::setHubRadius );
    connect( firstRangeFromSlider,
             &QSlider::valueChanged,
             firstRange,
             &ExRadialGaugeRange::setFromValue );
    connect( firstRangeToSlider,
             &QSlider::valueChanged,
             firstRange,
             &ExRadialGaugeRange::setToValue );
    connect( secondRangeFromSlider,
             &QSlider::valueChanged,
             secondRange,
             &ExRadialGaugeRange::setFromValue );
    connect( secondRangeToSlider,
             &QSlider::valueChanged,
             secondRange,
             &ExRadialGaugeRange::setToValue );
    connect( thirdRangeFromSlider,
             &QSlider::valueChanged,
             thirdRange,
             &ExRadialGaugeRange::setFromValue );
    connect( thirdRangeToSlider,
             &QSlider::valueChanged,
             thirdRange,
             &ExRadialGaugeRange::setToValue );
    connect( accentColorButton,
             &ExColorPickerButton::selectedColorChanged,
             gauge,
             [gauge]( const QColor& color )
             {
                 setGaugeAccentColor( gauge, color );
             } );
    connect( trackColorButton,
             &ExColorPickerButton::selectedColorChanged,
             gauge,
             [gauge]( const QColor& color )
             {
                 setGaugeTrackColor( gauge, color );
             } );
    connect( needleColorButton,
             &ExColorPickerButton::selectedColorChanged,
             gauge,
             [gauge]( const QColor& color )
             {
                 gauge->setNeedleColor( color );
             } );
    connect( tickColorButton,
             &ExColorPickerButton::selectedColorChanged,
             gauge,
             [gauge]( const QColor& color )
             {
                 gauge->setTickColor( color );
             } );
    connect( labelColorButton,
             &ExColorPickerButton::selectedColorChanged,
             gauge,
             [gauge]( const QColor& color )
             {
                 gauge->setLabelColor( color );
             } );
    connect( valueColorButton,
             &ExColorPickerButton::selectedColorChanged,
             gauge,
             [gauge]( const QColor& color )
             {
                 gauge->setValueColor( color );
             } );
    connect( firstRangeColorButton,
             &ExColorPickerButton::selectedColorChanged,
             firstRange,
             [firstRange]( const QColor& color )
             {
                 firstRange->setColor( color );
             } );
    connect( secondRangeColorButton,
             &ExColorPickerButton::selectedColorChanged,
             secondRange,
             [secondRange]( const QColor& color )
             {
                 secondRange->setColor( color );
             } );
    connect( thirdRangeColorButton,
             &ExColorPickerButton::selectedColorChanged,
             thirdRange,
             [thirdRange]( const QColor& color )
             {
                 thirdRange->setColor( color );
             } );
    connect( disabledCheck, &QCheckBox::toggled, gauge, [gauge]( bool disabled )
             {
                 gauge->setEnabled( !disabled );
             } );
    connect( resetButton,
             &QPushButton::clicked,
             gauge,
             [=]
             {
                 scaleModeCombo->setCurrentIndex( scaleModeCombo->findData( ExRadialGauge::ProgressScale ) );
                 valueAnimationDurationSlider->setValue( 180 );
                 valueSlider->setValue( 210 );
                 interactiveCheck->setChecked( true );
                 valueVisibleCheck->setChecked( true );
                 valuePositionCombo->setCurrentIndex(
                     valuePositionCombo->findData( ExRadialGauge::BottomValue ) );
                 titleEdit->clear();
                 unitEdit->clear();
                 valueFontPixelSizeSlider->setValue( 0 );
                 majorTickCountSlider->setValue( 11 );
                 minorTickCountSlider->setValue( 4 );
                 scaleWidthSlider->setValue( 16 );
                 minimumAngleSlider->setValue( -135 );
                 maximumAngleSlider->setValue( 135 );
                 needleWidthSlider->setValue( 6 );
                 needleLengthSlider->setValue( 62 );
                 tickLengthSlider->setValue( 14 );
                 tickWidthSlider->setValue( 3 );
                 majorTickLengthSlider->setValue( 20 );
                 majorTickWidthSlider->setValue( 4 );
                 scalePaddingSlider->setValue( 24 );
                 trackCapStyleCombo->setCurrentIndex( trackCapStyleCombo->findData( Qt::FlatCap ) );
                 ringCapStyleCombo->setCurrentIndex( ringCapStyleCombo->findData( Qt::FlatCap ) );
                 tickPaddingSlider->setValue( 16 );
                 labelsVisibleCheck->setChecked( false );
                 labelPaddingSlider->setValue( 28 );
                 labelFontPixelSizeSlider->setValue( 11 );
                 needleStyleCombo->setCurrentIndex(
                     needleStyleCombo->findData( ExRadialGauge::LineNeedle ) );
                 hubVisibleCheck->setChecked( false );
                 hubRadiusSlider->setValue( 10 );
                 firstRangeFromSlider->setValue( 0 );
                 firstRangeToSlider->setValue( 80 );
                 secondRangeFromSlider->setValue( 80 );
                 secondRangeToSlider->setValue( 160 );
                 thirdRangeFromSlider->setValue( 160 );
                 thirdRangeToSlider->setValue( 240 );
                 disabledCheck->setChecked( false );

                 gauge->setPalette( QPalette() );
                 gauge->setNeedleColor( QColor() );
                 gauge->setTickColor( QColor() );
                 gauge->setLabelColor( QColor() );
                 gauge->setValueColor( QColor() );
                 firstRange->setColor( firstRangeDefaultColor );
                 secondRange->setColor( secondRangeDefaultColor );
                 thirdRange->setColor( thirdRangeDefaultColor );
                 const QSignalBlocker accentBlocker( accentColorButton );
                 const QSignalBlocker trackBlocker( trackColorButton );
                 const QSignalBlocker needleBlocker( needleColorButton );
                 const QSignalBlocker tickBlocker( tickColorButton );
                 const QSignalBlocker labelBlocker( labelColorButton );
                 const QSignalBlocker valueBlocker( valueColorButton );
                 const QSignalBlocker firstRangeBlocker( firstRangeColorButton );
                 const QSignalBlocker secondRangeBlocker( secondRangeColorButton );
                 const QSignalBlocker thirdRangeBlocker( thirdRangeColorButton );
                 accentColorButton->setSelectedColor( gaugeAccentColor( gauge ) );
                 trackColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Mid ) );
                 needleColorButton->setSelectedColor( gaugeAccentColor( gauge ) );
                 tickColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
                 labelColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
                 valueColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
                 firstRangeColorButton->setSelectedColor( firstRangeDefaultColor );
                 secondRangeColorButton->setSelectedColor( secondRangeDefaultColor );
                 thirdRangeColorButton->setSelectedColor( thirdRangeDefaultColor );
                 updateRangeEditorState();
                 updateLabelEditorState();
                 updateNeedleEditorState();
                 updateValueEditorState();
                 gauge->update();
             } );

    updateRangeEditorState();
    updateLabelEditorState();
    updateNeedleEditorState();
    updateValueEditorState();

    auto* code = new QPlainTextEdit( propertiesCard );
    code->setReadOnly( true );
    code->setMaximumHeight( 150 );
    code->setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
    code->setPlainText( QStringLiteral(
        "auto *gauge = new ExRadialGauge(parent);\n"
        "gauge->setRange(0, 100);\n"
        "gauge->setValue(70);\n"
        "gauge->setMinimumAngle(-135.0);\n"
        "gauge->setMaximumAngle(135.0);\n"
        "gauge->setScaleMode(ExRadialGauge::RangeScale);\n"
        "gauge->addRange(0, 60, QColor(\"#21BCE2\"));\n"
        "gauge->addRange(60, 80, QColor(\"#FFB900\"));\n"
        "gauge->addRange(80, 100, QColor(\"#FF6475\"));" ) );
    propertiesLayout->addWidget( code );

    mainLayout->addWidget( propertiesCard );
    mainLayout->addStretch();
    scrollArea->setWidget( content );
}
