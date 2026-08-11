#include "radialgaugeshowcasewidget.h"

#include "fluentui3styleproperties.h"

#include <excolorpickerbutton.h>
#include <excombobox.h>
#include <exmultiprogressring.h>
#include <exmultiradialgauge.h>
#include <exradialgauge.h>
#include <extabwidget.h>

#include <QCheckBox>
#include <QComboBox>
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

void hideGaugeTrack( ExRadialGauge* gauge )
{
    setGaugeTrackColor( gauge, QColor( 0, 0, 0, 0 ) );
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

template<typename Widget, typename Setter>
void connectScaledSlider( QSlider* slider, Widget* widget, qreal scale, Setter setter )
{
    QObject::connect( slider, &QSlider::valueChanged, widget, [widget, scale, setter]( int value )
                     {
                         ( widget->*setter )( value / scale );
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
    gauge->setValueFontPixelSize( 23 );
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
    gauge->setProgressGradientEnabled( true );
    gauge->setSweepAreaVisible( true );
    gauge->setProgressGradientStartColor( QColor( QStringLiteral( "#38D8FF" ) ) );
    gauge->setProgressGradientEndColor( purple );

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

void configureEChartsBaseGauge( ExRadialGauge* gauge )
{
    gauge->setRange( 0, 100 );
    gauge->setValue( 50 );
    gauge->setMinimumAngle( -140.0 );
    gauge->setMaximumAngle( 140.0 );
    gauge->setScaleMode( ExRadialGauge::TrackScale );
    gauge->setScaleWidth( 5.0 );
    gauge->setMajorTickCount( 11 );
    gauge->setMinorTickCount( 4 );
    gauge->setTickLength( 4.0 );
    gauge->setTickWidth( 1.0 );
    gauge->setMajorTickLength( 7.0 );
    gauge->setMajorTickWidth( 1.4 );
    gauge->setTickPadding( 8.0 );
    gauge->setLabelsVisible( true );
    gauge->setLabelPadding( 20.0 );
    gauge->setLabelFontPixelSize( 9 );
    gauge->setNeedleStyle( ExRadialGauge::TriangleNeedle );
    gauge->setNeedleWidth( 8.0 );
    gauge->setNeedleLength( 0.58 );
    gauge->setHubVisible( false );
    gauge->setValuePosition( ExRadialGauge::CenterValue );
    gauge->setValueFontPixelSize( 20 );
    gauge->setValueAnimationDuration( 240 );
    gauge->setInteractive( true );
}

void configureEChartsBasicGauge( ExRadialGauge* gauge )
{
    const QColor blue( QStringLiteral( "#5470C6" ) );
    configureEChartsBaseGauge( gauge );
    gauge->setTitle( QStringLiteral( "SCORE" ) );
    gauge->setNeedleColor( blue );
    setGaugeAccentColor( gauge, blue );
    hideGaugeTrack( gauge );
}

void configureEChartsSimpleGauge( ExRadialGauge* gauge )
{
    const QColor blue( QStringLiteral( "#5470C6" ) );
    configureEChartsBaseGauge( gauge );
    gauge->setValue( 50 );
    gauge->setScaleMode( ExRadialGauge::ProgressScale );
    gauge->setScaleWidth( 6.0 );
    gauge->setTitle( QStringLiteral( "SCORE" ) );
    gauge->setNeedleStyle( ExRadialGauge::TriangleNeedle );
    gauge->setNeedleWidth( 8.0 );
    gauge->setNeedleColor( blue );
    setGaugeAccentColor( gauge, blue );
    hideGaugeTrack( gauge );
}

void configureEChartsSpeedGauge( ExRadialGauge* gauge )
{
    const QColor blue( QStringLiteral( "#5470C6" ) );
    configureEChartsBaseGauge( gauge );
    gauge->setValue( 70 );
    gauge->setScaleMode( ExRadialGauge::ProgressScale );
    gauge->setScaleWidth( 10.0 );
    gauge->setNeedleStyle( ExRadialGauge::LineNeedle );
    gauge->setNeedleWidth( 3.0 );
    gauge->setNeedleColor( blue );
    gauge->setHubVisible( true );
    gauge->setHubRadius( 6.0 );
    gauge->setValuePosition( ExRadialGauge::BottomValue );
    gauge->setValueFontPixelSize( 32 );
    setGaugeAccentColor( gauge, blue );
    hideGaugeTrack( gauge );
}

void configureEChartsProgressGauge( ExRadialGauge* gauge )
{
    const QColor cyan( QStringLiteral( "#45D1F5" ) );
    configureEChartsBaseGauge( gauge );
    gauge->setRange( 0, 240 );
    gauge->setValue( 100 );
    gauge->setMinimumAngle( -110.0 );
    gauge->setMaximumAngle( 110.0 );
    gauge->setScaleMode( ExRadialGauge::ProgressScale );
    gauge->setScaleWidth( 9.0 );
    gauge->setMajorTickCount( 13 );
    gauge->setNeedleStyle( ExRadialGauge::LineNeedle );
    gauge->setNeedleWidth( 5.0 );
    gauge->setNeedleLength( 0.64 );
    gauge->setNeedleColor( cyan );
    gauge->setHubVisible( false );
    gauge->setUnit( QStringLiteral( "km/h" ) );
    gauge->setValuePosition( ExRadialGauge::BottomValue );
    gauge->setValueFontPixelSize( 18 );
    setGaugeAccentColor( gauge, cyan );
    hideGaugeTrack( gauge );
}

void configureEChartsStageGauge( ExRadialGauge* gauge )
{
    const QColor cyan( QStringLiteral( "#42D0D0" ) );
    configureEChartsBaseGauge( gauge );
    gauge->setValue( 70 );
    gauge->setScaleMode( ExRadialGauge::RangeScale );
    gauge->setScaleWidth( 13.0 );
    gauge->clearRanges();
    gauge->addRange( 0, 70, cyan );
    gauge->addRange( 70, 80, QColor( QStringLiteral( "#3AA7D8" ) ) );
    gauge->addRange( 80, 100, QColor( QStringLiteral( "#FF6B75" ) ) );
    gauge->setNeedleStyle( ExRadialGauge::LineNeedle );
    gauge->setNeedleWidth( 4.0 );
    gauge->setNeedleColor( cyan );
    gauge->setUnit( QStringLiteral( "km/h" ) );
    gauge->setValuePosition( ExRadialGauge::CenterValue );
    gauge->setValueFontPixelSize( 17 );
    gauge->setValueColor( cyan );
    hideGaugeTrack( gauge );
}

void configureEChartsGradeGauge( ExRadialGauge* gauge )
{
    const QColor cyan( QStringLiteral( "#45D2E7" ) );
    configureEChartsBaseGauge( gauge );
    gauge->setValue( 70 );
    gauge->setMinimumAngle( -90.0 );
    gauge->setMaximumAngle( 90.0 );
    gauge->setScaleMode( ExRadialGauge::RangeScale );
    gauge->setScaleWidth( 4.0 );
    gauge->clearRanges();
    gauge->addRange( 0, 25, QColor( QStringLiteral( "#FF6475" ) ) );
    gauge->addRange( 25, 50, QColor( QStringLiteral( "#F3C74F" ) ) );
    gauge->addRange( 50, 75, cyan );
    gauge->addRange( 75, 100, QColor( QStringLiteral( "#61DDAA" ) ) );
    gauge->setMajorTickCount( 9 );
    gauge->setMinorTickCount( 3 );
    gauge->setLabelsVisible( false );
    gauge->setNeedleStyle( ExRadialGauge::TriangleNeedle );
    gauge->setNeedleWidth( 10.0 );
    gauge->setNeedleLength( 0.6 );
    gauge->setNeedleColor( cyan );
    gauge->setTitle( QStringLiteral( "Grade Rating" ) );
    gauge->setValuePosition( ExRadialGauge::CenterValue );
    gauge->setValueFontPixelSize( 18 );
    hideGaugeTrack( gauge );
}

void configureEChartsTemperatureGauge( ExRadialGauge* gauge )
{
    const QColor coral( QStringLiteral( "#FF9678" ) );
    configureEChartsBaseGauge( gauge );
    gauge->setRange( 0, 60 );
    gauge->setValue( 20 );
    gauge->setMinimumAngle( -110.0 );
    gauge->setMaximumAngle( 110.0 );
    gauge->setScaleMode( ExRadialGauge::ProgressScale );
    gauge->setScaleWidth( 12.0 );
    gauge->setMajorTickCount( 13 );
    gauge->setNeedleStyle( ExRadialGauge::NoNeedle );
    gauge->setUnit( QStringLiteral( "°C" ) );
    gauge->setValuePosition( ExRadialGauge::CenterValue );
    gauge->setValueFontPixelSize( 28 );
    gauge->setValueColor( coral );
    setGaugeAccentColor( gauge, coral );
    hideGaugeTrack( gauge );
}

void configureEChartsMultiTitleGauge( ExMultiRadialGauge* gauge )
{
    gauge->setRange( 0.0, 100.0 );
    gauge->setMinimumAngle( -140.0 );
    gauge->setMaximumAngle( 140.0 );
    gauge->setMajorTickCount( 11 );
    gauge->setMinorTickCount( 4 );
    gauge->setTrackWidth( 7.0 );
    gauge->setProgressWidth( 7.0 );
    gauge->setTrackCapStyle( Qt::RoundCap );
    gauge->setProgressCapStyle( Qt::RoundCap );
    gauge->setProgressOverlap( true );
    gauge->setScalePadding( 13.0 );
    gauge->setTickLength( 3.0 );
    gauge->setMajorTickLength( 6.0 );
    gauge->setTickPadding( 8.0 );
    gauge->setLabelPadding( 17.0 );
    gauge->setLabelFontPixelSize( 8 );
    gauge->setNeedleStyle( ExMultiRadialGauge::LineNeedle );
    gauge->setNeedleWidth( 4.0 );
    gauge->setNeedleLength( 0.68 );
    gauge->setNeedleOffset( QPointF( 0.0, 0.08 ) );
    gauge->setHubVisible( true );
    gauge->setHubRadius( 6.0 );
    gauge->setHubColor( QColor( QStringLiteral( "#FAC858" ) ) );
    gauge->setTitleFontPixelSize( 9 );
    gauge->setDetailFontPixelSize( 9 );
    gauge->setDetailBadgeVisible( true );
    gauge->setDetailBadgePadding( 6.0 );
    gauge->setValueSuffix( QStringLiteral( "%" ) );
    gauge->setValueAnimationDuration( 240 );
}

void configureEChartsBarometerGauge( ExRadialGauge* gauge )
{
    const QColor red( QStringLiteral( "#E63746" ) );
    configureEChartsBaseGauge( gauge );
    gauge->setValue( 40 );
    gauge->setMinimumAngle( -130.0 );
    gauge->setMaximumAngle( 130.0 );
    gauge->setScaleWidth( 2.0 );
    gauge->setTickColor( red );
    gauge->setLabelColor( red );
    gauge->setNeedleStyle( ExRadialGauge::LineNeedle );
    gauge->setNeedleWidth( 2.0 );
    gauge->setNeedleLength( 0.72 );
    gauge->setNeedleColor( gauge->palette().color( QPalette::Text ) );
    gauge->setHubVisible( true );
    gauge->setHubRadius( 3.0 );
    gauge->setTitle( QStringLiteral( "PLP" ) );
    gauge->setValuePosition( ExRadialGauge::CenterValue );
    gauge->setValueFontPixelSize( 16 );
    setGaugeTrackColor( gauge, red );
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

    auto* echartsCard = makeCard( content );
    auto* echartsLayout = new QVBoxLayout( echartsCard );
    echartsLayout->setContentsMargins( 16, 16, 16, 16 );
    echartsLayout->setSpacing( 12 );
    echartsLayout->addWidget( makeSectionTitle( tr( "ECharts 仪表盘配置" ), echartsCard ) );

    auto* echartsDescription = new QLabel(
        tr( "单值示例由 ExRadialGauge 的属性组合；多标题示例使用 ExMultiRadialGauge，共享刻度并绘制多条进度和指针。" ),
        echartsCard );
    echartsDescription->setWordWrap( true );
    echartsLayout->addWidget( echartsDescription );

    auto* echartsGrid = new QGridLayout;
    echartsGrid->setHorizontalSpacing( 20 );
    echartsGrid->setVerticalSpacing( 18 );
    for ( int column = 0; column < 5; ++column )
    {
        echartsGrid->setColumnStretch( column, 1 );
    }

    int echartsSampleIndex = 0;
    QList<ExRadialGauge*> echartsGauges;
    const auto addEChartsSample = [&]( const QString& title,
                                       const QString& subtitle,
                                       const auto& createVisual )
    {
        auto* sample = new QWidget( echartsCard );
        auto* sampleLayout = new QVBoxLayout( sample );
        sampleLayout->setContentsMargins( 4, 4, 4, 4 );
        sampleLayout->setSpacing( 4 );

        QWidget* visual = createVisual( sample );
        sampleLayout->addWidget( visual, 0, Qt::AlignHCenter );

        auto* sampleTitle = new QLabel( title, sample );
        QFont sampleTitleFont = sampleTitle->font();
        sampleTitleFont.setBold( true );
        sampleTitle->setFont( sampleTitleFont );
        sampleTitle->setAlignment( Qt::AlignHCenter );
        sampleLayout->addWidget( sampleTitle );

        auto* sampleSubtitle = new QLabel( subtitle, sample );
        sampleSubtitle->setProperty( "isSecondaryText", true );
        sampleSubtitle->setAlignment( Qt::AlignHCenter );
        sampleLayout->addWidget( sampleSubtitle );

        const int row = echartsSampleIndex / 5;
        const int column = echartsSampleIndex % 5;
        echartsGrid->addWidget( sample, row, column );
        ++echartsSampleIndex;
    };

    const auto addEChartsGauge = [&]( const QString& title,
                                      const QString& subtitle,
                                      void ( *configure )( ExRadialGauge* ) )
    {
        addEChartsSample( title, subtitle, [configure, &echartsGauges]( QWidget* parent ) -> QWidget*
                          {
                              auto* gauge = new ExRadialGauge( parent );
                              gauge->setFixedSize( 220, 220 );
                              configure( gauge );
                              echartsGauges.append( gauge );
                              return gauge;
                          } );
    };

    addEChartsGauge( tr( "基础仪表盘" ),
                       QStringLiteral( "Gauge Basic chart" ),
                       configureEChartsBasicGauge );
    addEChartsGauge( tr( "简单仪表盘" ),
                       QStringLiteral( "Simple Gauge" ),
                       configureEChartsSimpleGauge );
    addEChartsGauge( tr( "速度仪表盘" ),
                       QStringLiteral( "Speed Gauge" ),
                       configureEChartsSpeedGauge );
    addEChartsGauge( tr( "进度仪表盘" ),
                       QStringLiteral( "Progress Gauge" ),
                       configureEChartsProgressGauge );
    addEChartsGauge( tr( "阶段速度仪表盘" ),
                       QStringLiteral( "Stage Speed Gauge" ),
                       configureEChartsStageGauge );
    addEChartsGauge( tr( "等级仪表盘" ),
                       QStringLiteral( "Grade Gauge" ),
                       configureEChartsGradeGauge );

    ExMultiRadialGaugeItem* goodGaugeItem = nullptr;
    ExMultiRadialGaugeItem* betterGaugeItem = nullptr;
    ExMultiRadialGaugeItem* perfectGaugeItem = nullptr;
    addEChartsSample( tr( "多标题仪表盘" ),
                       QStringLiteral( "Multi Title Gauge" ),
                       [&goodGaugeItem,
                        &betterGaugeItem,
                        &perfectGaugeItem]( QWidget* parent ) -> QWidget*
                      {
                          auto* gauge = new ExMultiRadialGauge( parent );
                          gauge->setFixedSize( 220, 220 );
                          configureEChartsMultiTitleGauge( gauge );
                          goodGaugeItem = gauge->addItem( QStringLiteral( "Good" ),
                                                         20.0,
                                                         QColor( QStringLiteral( "#5470C6" ) ) );
                          betterGaugeItem = gauge->addItem( QStringLiteral( "Better" ),
                                                           40.0,
                                                           QColor( QStringLiteral( "#B8DE29" ) ) );
                          perfectGaugeItem = gauge->addItem( QStringLiteral( "Perfect" ),
                                                            60.0,
                                                            QColor( QStringLiteral( "#555672" ) ) );
                          goodGaugeItem->setTitleOffset( QPointF( -0.4, 0.8 ) );
                          goodGaugeItem->setDetailOffset( QPointF( -0.4, 0.95 ) );
                          betterGaugeItem->setTitleOffset( QPointF( 0.0, 0.8 ) );
                          betterGaugeItem->setDetailOffset( QPointF( 0.0, 0.95 ) );
                          perfectGaugeItem->setTitleOffset( QPointF( 0.4, 0.8 ) );
                          perfectGaugeItem->setDetailOffset( QPointF( 0.4, 0.95 ) );
                          return gauge;
                      } );

    addEChartsGauge( tr( "气温仪表盘" ),
                       QStringLiteral( "Temperature Gauge" ),
                       configureEChartsTemperatureGauge );

    ExMultiProgressRingItem* perfectRingItem = nullptr;
    ExMultiProgressRingItem* goodRingItem = nullptr;
    ExMultiProgressRingItem* commonRingItem = nullptr;
    addEChartsSample( tr( "得分环" ),
                       QStringLiteral( "Ring Gauge" ),
                       [&perfectRingItem,
                        &goodRingItem,
                        &commonRingItem]( QWidget* parent ) -> QWidget*
                      {
                          auto* ring = new ExMultiProgressRing( parent );
                          ring->setFixedSize( 220, 220 );
                          ring->setRingWidth( 8.0 );
                          ring->setRingSpacing( 6.0 );
                          ring->setRingPadding( 13.0 );
                          ring->setCapStyle( Qt::RoundCap );
                          ring->setTrackVisible( false );
                          perfectRingItem = ring->addItem( QStringLiteral( "Perfect" ),
                                                          20.0,
                                                          QColor( QStringLiteral( "#5470C6" ) ) );
                          goodRingItem = ring->addItem( QStringLiteral( "Good" ),
                                                       40.0,
                                                       QColor( QStringLiteral( "#B8DE29" ) ) );
                          commonRingItem = ring->addItem( QStringLiteral( "Commonly" ),
                                                         60.0,
                                                         QColor( QStringLiteral( "#5C5F7A" ) ) );
                          return ring;
                      } );

    addEChartsGauge( tr( "气压表" ),
                       QStringLiteral( "Gauge Barometer chart" ),
                       configureEChartsBarometerGauge );

    echartsLayout->addLayout( echartsGrid );

    auto* echartsValueLayout = new QHBoxLayout;
    auto* echartsValueLabel = new QLabel( tr( "公共百分比（拖动后同步）" ), echartsCard );
    auto* echartsValueSlider = makeValueSlider( echartsCard, 0, 100, 60 );
    echartsValueLayout->addWidget( echartsValueLabel );
    echartsValueLayout->addWidget( echartsValueSlider, 1 );
    echartsLayout->addLayout( echartsValueLayout );

    for ( ExRadialGauge* gauge : std::as_const( echartsGauges ) )
    {
        connect( echartsValueSlider,
                 &QSlider::valueChanged,
                 gauge,
                 [gauge]( int percent )
                 {
                     const qreal range = static_cast<qreal>( gauge->maximum() ) - gauge->minimum();
                     gauge->setValue( gauge->minimum() + qRound( range * percent / 100.0 ) );
                 } );
    }
    connect( echartsValueSlider, &QSlider::valueChanged, echartsCard, [=]( int percent )
             {
                 perfectRingItem->setValue( qMax( 0, percent - 40 ) );
                 goodRingItem->setValue( qMax( 0, percent - 20 ) );
                 commonRingItem->setValue( percent );
                 goodGaugeItem->setValue( qMax( 0, percent - 40 ) );
                 betterGaugeItem->setValue( qMax( 0, percent - 20 ) );
                 perfectGaugeItem->setValue( percent );
             } );
    mainLayout->addWidget( echartsCard );

    auto* propertiesCard = makeCard( content );
    auto* propertiesLayout = new QVBoxLayout( propertiesCard );
    propertiesLayout->setContentsMargins( 16, 16, 16, 16 );
    propertiesLayout->setSpacing( 12 );
    propertiesLayout->addWidget( makeSectionTitle( tr( "实时属性" ), propertiesCard ) );

    auto* livePropertyTabs = new ExTabWidget( propertiesCard );
    livePropertyTabs->tabBar()->setProperty( TabBarStyleProperty, TabBarStyle::Pivot_Slide );
    livePropertyTabs->setMinimumHeight( 640 );

    auto* gaugePropertyPage = new QWidget( livePropertyTabs );
    auto* gaugePropertyLayout = new QVBoxLayout( gaugePropertyPage );
    gaugePropertyLayout->setContentsMargins( 12, 12, 12, 12 );
    gaugePropertyLayout->setSpacing( 12 );

    auto* ringPropertyPage = new QWidget( livePropertyTabs );
    auto* ringPropertyLayout = new QVBoxLayout( ringPropertyPage );
    ringPropertyLayout->setContentsMargins( 12, 12, 12, 12 );
    ringPropertyLayout->setSpacing( 12 );

    auto* multiGaugePropertyPage = new QWidget( livePropertyTabs );
    auto* multiGaugePropertyLayout = new QVBoxLayout( multiGaugePropertyPage );
    multiGaugePropertyLayout->setContentsMargins( 12, 12, 12, 12 );
    multiGaugePropertyLayout->setSpacing( 12 );

    livePropertyTabs->addTab( gaugePropertyPage, QStringLiteral( "ExRadialGauge" ) );
    livePropertyTabs->addTab( ringPropertyPage, tr( "得分环" ) );
    livePropertyTabs->addTab( multiGaugePropertyPage, QStringLiteral( "ExMultiRadialGauge" ) );
    propertiesLayout->addWidget( livePropertyTabs );

    auto* previewLayout = new QHBoxLayout;
    previewLayout->setSpacing( 32 );

    auto* gauge = new ExRadialGauge( propertiesCard );
    gauge->setObjectName( QStringLiteral( "radialGaugePreview" ) );
    gauge->setRange( 0, 240 );
    gauge->setValue( 210 );
    gauge->setMajorTickCount( 10 );
    gauge->setMinorTickCount( 0 );
    gauge->setMajorTickLength( 7.0 );
    gauge->setNeedleWidth( 5.0 );
    gauge->setSingleStep( 1 );
    gauge->setPageStep( 10 );
    gauge->setFixedSize( 300, 300 );
    const QColor firstRangeDefaultColor( QStringLiteral( "#21BCE2" ) );
    const QColor secondRangeDefaultColor( QStringLiteral( "#FFB900" ) );
    const QColor thirdRangeDefaultColor( QStringLiteral( "#FF6475" ) );
    auto* firstRange = gauge->addRange( 0, 80, firstRangeDefaultColor );
    auto* secondRange = gauge->addRange( 80, 160, secondRangeDefaultColor );
    auto* thirdRange = gauge->addRange( 160, 240, thirdRangeDefaultColor );
    auto* gaugePreviewHost = new QWidget( gaugePropertyPage );
    auto* gaugePreviewHostLayout = new QVBoxLayout( gaugePreviewHost );
    gaugePreviewHostLayout->setContentsMargins( 0, 0, 0, 0 );
    gaugePreviewHostLayout->addWidget( gauge, 0, Qt::AlignHCenter | Qt::AlignTop );
    gaugePreviewHostLayout->addStretch();
    previewLayout->addWidget( gaugePreviewHost, 1 );

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
    QFormLayout* gradientForm = nullptr;
    QFormLayout* rangeForm = nullptr;
    auto* basicPage = makePropertyForm( basicForm );
    auto* scalePage = makePropertyForm( scaleForm );
    auto* needlePage = makePropertyForm( needleForm );
    auto* gradientPage = makePropertyForm( gradientForm );
    auto* rangePage = makePropertyForm( rangeForm );
    propertyTabs->addTab( basicPage, tr( "基础" ) );
    propertyTabs->addTab( scalePage, tr( "刻度与标签" ) );
    propertyTabs->addTab( needlePage, tr( "指针与文本" ) );
    propertyTabs->addTab( gradientPage, tr( "进度渐变" ) );
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

    auto* progressGradientEnabledCheck = new QCheckBox( tr( "进度环使用渐变色" ), propertiesCard );
    progressGradientEnabledCheck->setChecked( gauge->isProgressGradientEnabled() );

    auto* sweepAreaVisibleCheck = new QCheckBox( tr( "显示指针扫过扇形" ), propertiesCard );
    sweepAreaVisibleCheck->setChecked( gauge->isSweepAreaVisible() );

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
    auto* sweepAreaOpacitySlider = makeValueSlider( propertiesCard,
                                                    0,
                                                    100,
                                                    qRound( gauge->sweepAreaOpacity() * 100.0 ),
                                                    1,
                                                    10,
                                                    100,
                                                    2 );

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
    auto* progressGradientStartColorButton = new ExColorPickerButton( propertiesCard );
    progressGradientStartColorButton->setSelectedColor(
        gauge->progressGradientStartColor().isValid()
            ? gauge->progressGradientStartColor()
            : gaugeAccentColor( gauge ).lighter( 135 ) );
    auto* progressGradientEndColorButton = new ExColorPickerButton( propertiesCard );
    progressGradientEndColorButton->setSelectedColor(
        gauge->progressGradientEndColor().isValid()
            ? gauge->progressGradientEndColor()
            : gaugeAccentColor( gauge ) );
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

    gradientForm->addRow( progressGradientEnabledCheck );
    gradientForm->addRow( sweepAreaVisibleCheck );
    gradientForm->addRow( tr( "扇形不透明度" ), sweepAreaOpacitySlider );
    gradientForm->addRow( tr( "起点颜色" ), progressGradientStartColorButton );
    gradientForm->addRow( tr( "终点颜色" ), progressGradientEndColorButton );

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
    gaugePropertyLayout->addLayout( previewLayout );

    auto* angleHint = new QLabel(
        tr( "角度以正上方为 0°，顺时针为正；起止角度相同表示完整的 360°。" ),
        propertiesCard );
    angleHint->setWordWrap( true );
    gaugePropertyLayout->addWidget( angleHint );

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
    const auto updateGradientEditorState = [=]
    {
        const bool progressMode = gauge->scaleMode() == ExRadialGauge::ProgressScale;
        progressGradientEnabledCheck->setEnabled( progressMode );
        sweepAreaVisibleCheck->setEnabled( progressMode );
        sweepAreaOpacitySlider->setEnabled( progressMode && sweepAreaVisibleCheck->isChecked() );
        const bool colorsEnabled = progressMode
                                   && ( progressGradientEnabledCheck->isChecked()
                                        || sweepAreaVisibleCheck->isChecked() );
        progressGradientStartColorButton->setEnabled( colorsEnabled );
        progressGradientEndColorButton->setEnabled( colorsEnabled );
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
                 updateGradientEditorState();
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
    connect( progressGradientEnabledCheck,
             &QCheckBox::toggled,
             gauge,
             &ExRadialGauge::setProgressGradientEnabled );
    connect( progressGradientEnabledCheck, &QCheckBox::toggled, gauge, [=]
             {
                 updateGradientEditorState();
             } );
    connect( sweepAreaVisibleCheck,
             &QCheckBox::toggled,
             gauge,
             &ExRadialGauge::setSweepAreaVisible );
    connect( sweepAreaVisibleCheck, &QCheckBox::toggled, gauge, [=]
             {
                 updateGradientEditorState();
             } );
    connectScaledSlider( sweepAreaOpacitySlider,
                         gauge,
                         100.0,
                         &ExRadialGauge::setSweepAreaOpacity );
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
    connect( gauge,
             &ExRadialGauge::trackCapStyleChanged,
             trackCapStyleCombo,
             [trackCapStyleCombo]( Qt::PenCapStyle style )
             {
                 const QSignalBlocker blocker( trackCapStyleCombo );
                 trackCapStyleCombo->setCurrentIndex( trackCapStyleCombo->findData( style ) );
             } );
    connect( gauge,
             &ExRadialGauge::ringCapStyleChanged,
             ringCapStyleCombo,
             [ringCapStyleCombo]( Qt::PenCapStyle style )
             {
                 const QSignalBlocker blocker( ringCapStyleCombo );
                 ringCapStyleCombo->setCurrentIndex( ringCapStyleCombo->findData( style ) );
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
    connect( progressGradientStartColorButton,
             &ExColorPickerButton::selectedColorChanged,
             gauge,
             &ExRadialGauge::setProgressGradientStartColor );
    connect( progressGradientEndColorButton,
             &ExColorPickerButton::selectedColorChanged,
             gauge,
             &ExRadialGauge::setProgressGradientEndColor );
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
                 progressGradientEnabledCheck->setChecked( false );
                 sweepAreaVisibleCheck->setChecked( false );
                 sweepAreaOpacitySlider->setValue( 16 );
                 valueVisibleCheck->setChecked( true );
                 valuePositionCombo->setCurrentIndex(
                     valuePositionCombo->findData( ExRadialGauge::BottomValue ) );
                 titleEdit->clear();
                 unitEdit->clear();
                 valueFontPixelSizeSlider->setValue( 0 );
                 majorTickCountSlider->setValue( 10 );
                 minorTickCountSlider->setValue( 0 );
                 scaleWidthSlider->setValue( 16 );
                 minimumAngleSlider->setValue( -135 );
                 maximumAngleSlider->setValue( 135 );
                 needleWidthSlider->setValue( 10 );
                 needleLengthSlider->setValue( 62 );
                 tickLengthSlider->setValue( 14 );
                 tickWidthSlider->setValue( 3 );
                 majorTickLengthSlider->setValue( 14 );
                 majorTickWidthSlider->setValue( 4 );
                 scalePaddingSlider->setValue( 24 );
                 trackCapStyleCombo->setCurrentIndex( trackCapStyleCombo->findData( Qt::RoundCap ) );
                 ringCapStyleCombo->setCurrentIndex( ringCapStyleCombo->findData( Qt::RoundCap ) );
                 tickPaddingSlider->setValue( 16 );
                 labelsVisibleCheck->setChecked( false );
                 labelPaddingSlider->setValue( 56 );
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
                 gauge->setProgressGradientStartColor( QColor() );
                 gauge->setProgressGradientEndColor( QColor() );
                 firstRange->setColor( firstRangeDefaultColor );
                 secondRange->setColor( secondRangeDefaultColor );
                 thirdRange->setColor( thirdRangeDefaultColor );
                 const QSignalBlocker accentBlocker( accentColorButton );
                 const QSignalBlocker trackBlocker( trackColorButton );
                 const QSignalBlocker needleBlocker( needleColorButton );
                 const QSignalBlocker tickBlocker( tickColorButton );
                 const QSignalBlocker labelBlocker( labelColorButton );
                 const QSignalBlocker valueBlocker( valueColorButton );
                 const QSignalBlocker progressGradientStartBlocker( progressGradientStartColorButton );
                 const QSignalBlocker progressGradientEndBlocker( progressGradientEndColorButton );
                 const QSignalBlocker firstRangeBlocker( firstRangeColorButton );
                 const QSignalBlocker secondRangeBlocker( secondRangeColorButton );
                 const QSignalBlocker thirdRangeBlocker( thirdRangeColorButton );
                 accentColorButton->setSelectedColor( gaugeAccentColor( gauge ) );
                 trackColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Mid ) );
                 needleColorButton->setSelectedColor( gaugeAccentColor( gauge ) );
                 tickColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
                 labelColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
                 valueColorButton->setSelectedColor( gauge->palette().color( QPalette::Active, QPalette::Text ) );
                 progressGradientStartColorButton->setSelectedColor( gaugeAccentColor( gauge ).lighter( 135 ) );
                 progressGradientEndColorButton->setSelectedColor( gaugeAccentColor( gauge ) );
                 firstRangeColorButton->setSelectedColor( firstRangeDefaultColor );
                 secondRangeColorButton->setSelectedColor( secondRangeDefaultColor );
                 thirdRangeColorButton->setSelectedColor( thirdRangeDefaultColor );
                 updateRangeEditorState();
                 updateGradientEditorState();
                 updateLabelEditorState();
                 updateNeedleEditorState();
                 updateValueEditorState();
                 gauge->update();
             } );

    updateRangeEditorState();
    updateGradientEditorState();
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
    gaugePropertyLayout->addWidget( code );

    auto* ringPreviewLayout = new QHBoxLayout;
    ringPreviewLayout->setSpacing( 32 );

    auto* ring = new ExMultiProgressRing( ringPropertyPage );
    ring->setObjectName( QStringLiteral( "multiProgressRingPreview" ) );
    ring->setFixedSize( 300, 300 );
    ring->setRingWidth( 8.0 );
    ring->setRingSpacing( 6.0 );
    ring->setRingPadding( 13.0 );
    ring->setCapStyle( Qt::RoundCap );
    ring->setTrackVisible( false );
    const QColor perfectDefaultColor( QStringLiteral( "#5470C6" ) );
    const QColor goodDefaultColor( QStringLiteral( "#B8DE29" ) );
    const QColor commonDefaultColor( QStringLiteral( "#5C5F7A" ) );
    auto* perfectItem = ring->addItem( QStringLiteral( "Perfect" ), 20.0, perfectDefaultColor );
    auto* goodItem = ring->addItem( QStringLiteral( "Good" ), 40.0, goodDefaultColor );
    auto* commonItem = ring->addItem( QStringLiteral( "Commonly" ), 60.0, commonDefaultColor );
    auto* ringPreviewHost = new QWidget( ringPropertyPage );
    auto* ringPreviewHostLayout = new QVBoxLayout( ringPreviewHost );
    ringPreviewHostLayout->setContentsMargins( 0, 0, 0, 0 );
    ringPreviewHostLayout->addWidget( ring, 0, Qt::AlignHCenter | Qt::AlignTop );
    ringPreviewHostLayout->addStretch();
    ringPreviewLayout->addWidget( ringPreviewHost, 1 );

    auto* ringEditorTabs = new ExTabWidget( ringPropertyPage );
    ringEditorTabs->tabBar()->setProperty( TabBarStyleProperty, TabBarStyle::Pivot_Slide );
    ringEditorTabs->setMinimumWidth( 440 );
    ringEditorTabs->setMinimumHeight( 430 );

    const auto makeRingPropertyForm = [ringEditorTabs]( QFormLayout*& form )
    {
        auto* page = new QWidget( ringEditorTabs );
        form = new QFormLayout( page );
        form->setContentsMargins( 12, 12, 12, 12 );
        form->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
        form->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
        form->setHorizontalSpacing( 12 );
        form->setVerticalSpacing( 10 );
        return page;
    };

    QFormLayout* ringLayoutForm = nullptr;
    QFormLayout* ringDataForm = nullptr;
    QFormLayout* ringDetailsForm = nullptr;
    auto* ringLayoutPage = makeRingPropertyForm( ringLayoutForm );
    auto* ringDataPage = makeRingPropertyForm( ringDataForm );
    auto* ringDetailsPage = makeRingPropertyForm( ringDetailsForm );
    ringEditorTabs->addTab( ringLayoutPage, tr( "环与布局" ) );
    ringEditorTabs->addTab( ringDataPage, tr( "数据" ) );
    ringEditorTabs->addTab( ringDetailsPage, tr( "详情" ) );

    const auto makeRingMetricSlider = [ringPropertyPage]( qreal value, qreal minimum, qreal maximum )
    {
        constexpr int scale = 2;
        return makeValueSlider( ringPropertyPage,
                                qRound( minimum * scale ),
                                qRound( maximum * scale ),
                                qRound( value * scale ),
                                1,
                                4,
                                scale,
                                1 );
    };

    auto* ringMinimumSlider = makeValueSlider( ringPropertyPage, -100, 100, qRound( ring->minimum() ) );
    auto* ringMaximumSlider = makeValueSlider( ringPropertyPage, 1, 200, qRound( ring->maximum() ) );
    auto* ringStartAngleSlider = makeValueSlider( ringPropertyPage, -360, 360, qRound( ring->startAngle() ), 5, 15 );
    auto* ringSweepAngleSlider = makeValueSlider( ringPropertyPage, 0, 360, qRound( ring->sweepAngle() ), 5, 15 );
    auto* ringWidthSlider = makeRingMetricSlider( ring->ringWidth(), 0.5, 32.0 );
    auto* ringSpacingSlider = makeRingMetricSlider( ring->ringSpacing(), 0.0, 32.0 );
    auto* ringPaddingSlider = makeRingMetricSlider( ring->ringPadding(), 0.0, 80.0 );
    auto* ringAnimationDurationSlider = makeValueSlider( ringPropertyPage,
                                                         0,
                                                         2000,
                                                         ring->valueAnimationDuration(),
                                                         10,
                                                         100 );

    auto* scoreRingCapStyleCombo = new ExComboBox( ringPropertyPage );
    scoreRingCapStyleCombo->addItem( QStringLiteral( "FlatCap" ), Qt::FlatCap );
    scoreRingCapStyleCombo->addItem( QStringLiteral( "SquareCap" ), Qt::SquareCap );
    scoreRingCapStyleCombo->addItem( QStringLiteral( "RoundCap" ), Qt::RoundCap );
    scoreRingCapStyleCombo->setCurrentIndex( scoreRingCapStyleCombo->findData( ring->capStyle() ) );

    auto* ringTrackVisibleCheck = new QCheckBox( tr( "显示 Track" ), ringPropertyPage );
    ringTrackVisibleCheck->setChecked( ring->isTrackVisible() );
    auto* ringTrackColorButton = new ExColorPickerButton( ringPropertyPage );
    ringTrackColorButton->setSelectedColor( ring->palette().color( QPalette::Active, QPalette::Mid ) );
    ringTrackColorButton->setEnabled( ring->isTrackVisible() );

    ringLayoutForm->addRow( tr( "最小值" ), ringMinimumSlider );
    ringLayoutForm->addRow( tr( "最大值" ), ringMaximumSlider );
    ringLayoutForm->addRow( tr( "起始角度" ), ringStartAngleSlider );
    ringLayoutForm->addRow( tr( "扫过角度" ), ringSweepAngleSlider );
    ringLayoutForm->addRow( tr( "环宽度" ), ringWidthSlider );
    ringLayoutForm->addRow( tr( "环间距" ), ringSpacingSlider );
    ringLayoutForm->addRow( tr( "外圈边距" ), ringPaddingSlider );
    ringLayoutForm->addRow( tr( "端点样式" ), scoreRingCapStyleCombo );
    ringLayoutForm->addRow( ringTrackVisibleCheck );
    ringLayoutForm->addRow( tr( "Track 颜色" ), ringTrackColorButton );
    ringLayoutForm->addRow( tr( "数值动画时长" ), ringAnimationDurationSlider );

    auto* perfectLabelEdit = new QLineEdit( perfectItem->label(), ringPropertyPage );
    auto* goodLabelEdit = new QLineEdit( goodItem->label(), ringPropertyPage );
    auto* commonLabelEdit = new QLineEdit( commonItem->label(), ringPropertyPage );
    auto* perfectValueSlider = makeValueSlider( ringPropertyPage, 0, 100, qRound( perfectItem->value() ) );
    auto* goodValueSlider = makeValueSlider( ringPropertyPage, 0, 100, qRound( goodItem->value() ) );
    auto* commonValueSlider = makeValueSlider( ringPropertyPage, 0, 100, qRound( commonItem->value() ) );
    auto* perfectColorButton = new ExColorPickerButton( ringPropertyPage );
    perfectColorButton->setSelectedColor( perfectItem->color() );
    auto* goodColorButton = new ExColorPickerButton( ringPropertyPage );
    goodColorButton->setSelectedColor( goodItem->color() );
    auto* commonColorButton = new ExColorPickerButton( ringPropertyPage );
    commonColorButton->setSelectedColor( commonItem->color() );

    ringDataForm->addRow( tr( "项目 1 名称" ), perfectLabelEdit );
    ringDataForm->addRow( tr( "项目 1 数值" ), perfectValueSlider );
    ringDataForm->addRow( tr( "项目 1 颜色" ), perfectColorButton );
    ringDataForm->addRow( tr( "项目 2 名称" ), goodLabelEdit );
    ringDataForm->addRow( tr( "项目 2 数值" ), goodValueSlider );
    ringDataForm->addRow( tr( "项目 2 颜色" ), goodColorButton );
    ringDataForm->addRow( tr( "项目 3 名称" ), commonLabelEdit );
    ringDataForm->addRow( tr( "项目 3 数值" ), commonValueSlider );
    ringDataForm->addRow( tr( "项目 3 颜色" ), commonColorButton );

    auto* ringDetailsVisibleCheck = new QCheckBox( tr( "显示中央详情" ), ringPropertyPage );
    ringDetailsVisibleCheck->setChecked( ring->areDetailsVisible() );
    auto* ringBadgeVisibleCheck = new QCheckBox( tr( "数值使用徽标边框" ), ringPropertyPage );
    ringBadgeVisibleCheck->setChecked( ring->isValueBadgeVisible() );
    auto* ringValueSuffixEdit = new QLineEdit( ring->valueSuffix(), ringPropertyPage );
    auto* ringValueDecimalsSlider = makeValueSlider( ringPropertyPage, 0, 6, ring->valueDecimals() );
    auto* ringLabelFontSizeSlider = makeValueSlider( ringPropertyPage, 0, 48, ring->labelFontPixelSize() );
    auto* ringValueFontSizeSlider = makeValueSlider( ringPropertyPage, 0, 48, ring->valueFontPixelSize() );
    auto* ringLabelColorButton = new ExColorPickerButton( ringPropertyPage );
    ringLabelColorButton->setSelectedColor( ring->palette().color( QPalette::Active, QPalette::Text ) );

    ringDetailsForm->addRow( ringDetailsVisibleCheck );
    ringDetailsForm->addRow( ringBadgeVisibleCheck );
    ringDetailsForm->addRow( tr( "数值后缀" ), ringValueSuffixEdit );
    ringDetailsForm->addRow( tr( "数值小数位" ), ringValueDecimalsSlider );
    ringDetailsForm->addRow( tr( "名称字号（0 自动）" ), ringLabelFontSizeSlider );
    ringDetailsForm->addRow( tr( "数值字号（0 自动）" ), ringValueFontSizeSlider );
    ringDetailsForm->addRow( tr( "名称颜色" ), ringLabelColorButton );

    connect( ringMinimumSlider, &QSlider::valueChanged, ring, [=]( int minimum )
             {
                 if ( minimum >= ringMaximumSlider->value() )
                 {
                     ringMaximumSlider->setValue( minimum + 1 );
                 }
                 ring->setMinimum( minimum );
                 perfectValueSlider->setMinimum( minimum );
                 goodValueSlider->setMinimum( minimum );
                 commonValueSlider->setMinimum( minimum );
             } );
    connect( ringMaximumSlider, &QSlider::valueChanged, ring, [=]( int maximum )
             {
                 if ( maximum <= ringMinimumSlider->value() )
                 {
                     ringMinimumSlider->setValue( maximum - 1 );
                 }
                 ring->setMaximum( maximum );
                 perfectValueSlider->setMaximum( maximum );
                 goodValueSlider->setMaximum( maximum );
                 commonValueSlider->setMaximum( maximum );
             } );
    connectScaledSlider( ringStartAngleSlider, ring, 1.0, &ExMultiProgressRing::setStartAngle );
    connectScaledSlider( ringSweepAngleSlider, ring, 1.0, &ExMultiProgressRing::setSweepAngle );
    connectScaledSlider( ringWidthSlider, ring, 2.0, &ExMultiProgressRing::setRingWidth );
    connectScaledSlider( ringSpacingSlider, ring, 2.0, &ExMultiProgressRing::setRingSpacing );
    connectScaledSlider( ringPaddingSlider, ring, 2.0, &ExMultiProgressRing::setRingPadding );
    connectScaledSlider( ringAnimationDurationSlider,
                         ring,
                         1.0,
                         &ExMultiProgressRing::setValueAnimationDuration );
    connect( scoreRingCapStyleCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             ring,
             [=]( int )
             {
                 ring->setCapStyle(
                     static_cast<Qt::PenCapStyle>( scoreRingCapStyleCombo->currentData().toInt() ) );
             } );
    connect( ringTrackVisibleCheck, &QCheckBox::toggled, ring, &ExMultiProgressRing::setTrackVisible );
    connect( ringTrackVisibleCheck, &QCheckBox::toggled, ringTrackColorButton, &QWidget::setEnabled );
    connect( ringTrackColorButton,
             &ExColorPickerButton::selectedColorChanged,
             ring,
             &ExMultiProgressRing::setTrackColor );

    connect( perfectLabelEdit, &QLineEdit::textChanged, perfectItem, &ExMultiProgressRingItem::setLabel );
    connect( goodLabelEdit, &QLineEdit::textChanged, goodItem, &ExMultiProgressRingItem::setLabel );
    connect( commonLabelEdit, &QLineEdit::textChanged, commonItem, &ExMultiProgressRingItem::setLabel );
    connectScaledSlider( perfectValueSlider, perfectItem, 1.0, &ExMultiProgressRingItem::setValue );
    connectScaledSlider( goodValueSlider, goodItem, 1.0, &ExMultiProgressRingItem::setValue );
    connectScaledSlider( commonValueSlider, commonItem, 1.0, &ExMultiProgressRingItem::setValue );
    connect( perfectColorButton,
             &ExColorPickerButton::selectedColorChanged,
             perfectItem,
             &ExMultiProgressRingItem::setColor );
    connect( goodColorButton,
             &ExColorPickerButton::selectedColorChanged,
             goodItem,
             &ExMultiProgressRingItem::setColor );
    connect( commonColorButton,
             &ExColorPickerButton::selectedColorChanged,
             commonItem,
             &ExMultiProgressRingItem::setColor );

    connect( ringDetailsVisibleCheck,
             &QCheckBox::toggled,
             ring,
             &ExMultiProgressRing::setDetailsVisible );
    connect( ringBadgeVisibleCheck,
             &QCheckBox::toggled,
             ring,
             &ExMultiProgressRing::setValueBadgeVisible );
    connect( ringValueSuffixEdit, &QLineEdit::textChanged, ring, &ExMultiProgressRing::setValueSuffix );
    connectScaledSlider( ringValueDecimalsSlider, ring, 1.0, &ExMultiProgressRing::setValueDecimals );
    connectScaledSlider( ringLabelFontSizeSlider, ring, 1.0, &ExMultiProgressRing::setLabelFontPixelSize );
    connectScaledSlider( ringValueFontSizeSlider, ring, 1.0, &ExMultiProgressRing::setValueFontPixelSize );
    connect( ringLabelColorButton,
             &ExColorPickerButton::selectedColorChanged,
             ring,
             &ExMultiProgressRing::setLabelColor );

    ringPreviewLayout->addWidget( ringEditorTabs, 1 );
    ringPropertyLayout->addLayout( ringPreviewLayout );
    auto* ringHint = new QLabel(
        tr( "每个数据项对应一条独立圆环；数值、名称和颜色均可单独设置，其余属性由控件统一管理。" ),
        ringPropertyPage );
    ringHint->setWordWrap( true );
    ringPropertyLayout->addWidget( ringHint );
    ringPropertyLayout->addStretch();

    auto* multiGaugePreviewLayout = new QHBoxLayout;
    multiGaugePreviewLayout->setSpacing( 32 );

    auto* multiGauge = new ExMultiRadialGauge( multiGaugePropertyPage );
    multiGauge->setObjectName( QStringLiteral( "multiRadialGaugePreview" ) );
    multiGauge->setFixedSize( 300, 300 );
    configureEChartsMultiTitleGauge( multiGauge );
    multiGauge->setTrackWidth( 9.0 );
    multiGauge->setProgressWidth( 9.0 );
    multiGauge->setNeedleWidth( 5.0 );
    multiGauge->setTitleFontPixelSize( 11 );
    multiGauge->setDetailFontPixelSize( 11 );
    const QColor multiGaugeGoodDefaultColor( QStringLiteral( "#5470C6" ) );
    const QColor multiGaugeBetterDefaultColor( QStringLiteral( "#B8DE29" ) );
    const QColor multiGaugePerfectDefaultColor( QStringLiteral( "#555672" ) );
    auto* multiGaugeGoodItem = multiGauge->addItem( QStringLiteral( "Good" ),
                                                    20.0,
                                                    multiGaugeGoodDefaultColor );
    auto* multiGaugeBetterItem = multiGauge->addItem( QStringLiteral( "Better" ),
                                                      40.0,
                                                      multiGaugeBetterDefaultColor );
    auto* multiGaugePerfectItem = multiGauge->addItem( QStringLiteral( "Perfect" ),
                                                       60.0,
                                                       multiGaugePerfectDefaultColor );
    multiGaugeGoodItem->setTitleOffset( QPointF( -0.4, 0.8 ) );
    multiGaugeGoodItem->setDetailOffset( QPointF( -0.4, 0.95 ) );
    multiGaugeBetterItem->setTitleOffset( QPointF( 0.0, 0.8 ) );
    multiGaugeBetterItem->setDetailOffset( QPointF( 0.0, 0.95 ) );
    multiGaugePerfectItem->setTitleOffset( QPointF( 0.4, 0.8 ) );
    multiGaugePerfectItem->setDetailOffset( QPointF( 0.4, 0.95 ) );

    auto* multiGaugePreviewHost = new QWidget( multiGaugePropertyPage );
    auto* multiGaugePreviewHostLayout = new QVBoxLayout( multiGaugePreviewHost );
    multiGaugePreviewHostLayout->setContentsMargins( 0, 0, 0, 0 );
    multiGaugePreviewHostLayout->addWidget( multiGauge, 0, Qt::AlignHCenter | Qt::AlignTop );
    multiGaugePreviewHostLayout->addStretch();
    multiGaugePreviewLayout->addWidget( multiGaugePreviewHost, 1 );

    auto* multiGaugeEditorTabs = new ExTabWidget( multiGaugePropertyPage );
    multiGaugeEditorTabs->tabBar()->setProperty( TabBarStyleProperty, TabBarStyle::Pivot_Slide );
    multiGaugeEditorTabs->setMinimumWidth( 440 );
    multiGaugeEditorTabs->setMinimumHeight( 430 );

    const auto makeMultiGaugePropertyForm = [multiGaugeEditorTabs]( QFormLayout*& form )
    {
        auto* page = new QWidget( multiGaugeEditorTabs );
        form = new QFormLayout( page );
        form->setContentsMargins( 12, 12, 12, 12 );
        form->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
        form->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
        form->setHorizontalSpacing( 12 );
        form->setVerticalSpacing( 8 );
        return page;
    };

    QFormLayout* multiGaugeRingForm = nullptr;
    QFormLayout* multiGaugeScaleForm = nullptr;
    QFormLayout* multiGaugeNeedleForm = nullptr;
    QFormLayout* multiGaugeDataForm = nullptr;
    QFormLayout* multiGaugePositionForm = nullptr;
    QFormLayout* multiGaugeTextForm = nullptr;
    auto* multiGaugeRingPage = makeMultiGaugePropertyForm( multiGaugeRingForm );
    auto* multiGaugeScalePage = makeMultiGaugePropertyForm( multiGaugeScaleForm );
    auto* multiGaugeNeedlePage = makeMultiGaugePropertyForm( multiGaugeNeedleForm );
    auto* multiGaugeDataPage = makeMultiGaugePropertyForm( multiGaugeDataForm );
    auto* multiGaugePositionPage = makeMultiGaugePropertyForm( multiGaugePositionForm );
    auto* multiGaugeTextPage = makeMultiGaugePropertyForm( multiGaugeTextForm );
    multiGaugeEditorTabs->addTab( multiGaugeRingPage, tr( "范围与环" ) );
    multiGaugeEditorTabs->addTab( multiGaugeScalePage, tr( "刻度" ) );
    multiGaugeEditorTabs->addTab( multiGaugeNeedlePage, tr( "指针" ) );
    multiGaugeEditorTabs->addTab( multiGaugeDataPage, tr( "数据" ) );
    multiGaugeEditorTabs->addTab( multiGaugePositionPage, tr( "位置" ) );
    multiGaugeEditorTabs->addTab( multiGaugeTextPage, tr( "文本" ) );

    const auto makeMultiGaugeMetricSlider = [multiGaugePropertyPage]( qreal value,
                                                                      qreal minimum,
                                                                      qreal maximum )
    {
        constexpr int scale = 2;
        return makeValueSlider( multiGaugePropertyPage,
                                qRound( minimum * scale ),
                                qRound( maximum * scale ),
                                qRound( value * scale ),
                                1,
                                4,
                                scale,
                                1 );
    };
    const auto makeMultiGaugeCapStyleCombo = [multiGaugePropertyPage]( Qt::PenCapStyle style )
    {
        auto* combo = new ExComboBox( multiGaugePropertyPage );
        combo->addItem( QStringLiteral( "FlatCap" ), Qt::FlatCap );
        combo->addItem( QStringLiteral( "SquareCap" ), Qt::SquareCap );
        combo->addItem( QStringLiteral( "RoundCap" ), Qt::RoundCap );
        combo->setCurrentIndex( combo->findData( style ) );
        return combo;
    };

    auto* multiGaugeMinimumSlider = makeValueSlider( multiGaugePropertyPage,
                                                      -100,
                                                      99,
                                                      qRound( multiGauge->minimum() ) );
    auto* multiGaugeMaximumSlider = makeValueSlider( multiGaugePropertyPage,
                                                      1,
                                                      200,
                                                      qRound( multiGauge->maximum() ) );
    auto* multiGaugeMinimumAngleSlider = makeValueSlider( multiGaugePropertyPage,
                                                           -360,
                                                           360,
                                                           qRound( multiGauge->minimumAngle() ),
                                                           5,
                                                           15 );
    auto* multiGaugeMaximumAngleSlider = makeValueSlider( multiGaugePropertyPage,
                                                           -360,
                                                           360,
                                                           qRound( multiGauge->maximumAngle() ),
                                                           5,
                                                           15 );
    auto* multiGaugeScalePaddingSlider = makeMultiGaugeMetricSlider( multiGauge->scalePadding(),
                                                                     0.0,
                                                                     80.0 );
    auto* multiGaugeTrackVisibleCheck = new QCheckBox( tr( "显示 Track" ), multiGaugePropertyPage );
    multiGaugeTrackVisibleCheck->setChecked( multiGauge->isTrackVisible() );
    auto* multiGaugeTrackWidthSlider = makeMultiGaugeMetricSlider( multiGauge->trackWidth(), 0.5, 32.0 );
    auto* multiGaugeTrackColorButton = new ExColorPickerButton( multiGaugePropertyPage );
    multiGaugeTrackColorButton->setSelectedColor(
        multiGauge->palette().color( QPalette::Active, QPalette::Mid ) );
    auto* multiGaugeTrackCapStyleCombo = makeMultiGaugeCapStyleCombo( multiGauge->trackCapStyle() );
    auto* multiGaugeProgressVisibleCheck = new QCheckBox( tr( "显示进度弧" ), multiGaugePropertyPage );
    multiGaugeProgressVisibleCheck->setChecked( multiGauge->isProgressVisible() );
    auto* multiGaugeProgressOverlapCheck = new QCheckBox( tr( "进度弧重叠" ), multiGaugePropertyPage );
    multiGaugeProgressOverlapCheck->setChecked( multiGauge->isProgressOverlap() );
    auto* multiGaugeProgressWidthSlider = makeMultiGaugeMetricSlider( multiGauge->progressWidth(),
                                                                      0.5,
                                                                      32.0 );
    auto* multiGaugeProgressSpacingSlider = makeMultiGaugeMetricSlider( multiGauge->progressSpacing(),
                                                                        0.0,
                                                                        32.0 );
    multiGaugeProgressSpacingSlider->setEnabled( !multiGauge->isProgressOverlap() );
    auto* multiGaugeProgressCapStyleCombo = makeMultiGaugeCapStyleCombo( multiGauge->progressCapStyle() );
    auto* multiGaugeAnimationSlider = makeValueSlider( multiGaugePropertyPage,
                                                        0,
                                                        2000,
                                                        multiGauge->valueAnimationDuration(),
                                                        10,
                                                        100 );

    multiGaugeRingForm->addRow( tr( "最小值" ), multiGaugeMinimumSlider );
    multiGaugeRingForm->addRow( tr( "最大值" ), multiGaugeMaximumSlider );
    multiGaugeRingForm->addRow( tr( "起始角度" ), multiGaugeMinimumAngleSlider );
    multiGaugeRingForm->addRow( tr( "结束角度" ), multiGaugeMaximumAngleSlider );
    multiGaugeRingForm->addRow( tr( "外圈边距" ), multiGaugeScalePaddingSlider );
    multiGaugeRingForm->addRow( multiGaugeTrackVisibleCheck );
    multiGaugeRingForm->addRow( tr( "Track 宽度" ), multiGaugeTrackWidthSlider );
    multiGaugeRingForm->addRow( tr( "Track 颜色" ), multiGaugeTrackColorButton );
    multiGaugeRingForm->addRow( tr( "Track 端点" ), multiGaugeTrackCapStyleCombo );
    multiGaugeRingForm->addRow( multiGaugeProgressVisibleCheck );
    multiGaugeRingForm->addRow( multiGaugeProgressOverlapCheck );
    multiGaugeRingForm->addRow( tr( "进度弧宽度" ), multiGaugeProgressWidthSlider );
    multiGaugeRingForm->addRow( tr( "同心弧间距" ), multiGaugeProgressSpacingSlider );
    multiGaugeRingForm->addRow( tr( "进度弧端点" ), multiGaugeProgressCapStyleCombo );
    multiGaugeRingForm->addRow( tr( "数值动画时长" ), multiGaugeAnimationSlider );

    auto* multiGaugeMajorTickCountSlider = makeValueSlider( multiGaugePropertyPage,
                                                            2,
                                                            100,
                                                            multiGauge->majorTickCount() );
    auto* multiGaugeMinorTickCountSlider = makeValueSlider( multiGaugePropertyPage,
                                                            0,
                                                            20,
                                                            multiGauge->minorTickCount() );
    auto* multiGaugeTickLengthSlider = makeMultiGaugeMetricSlider( multiGauge->tickLength(), 0.0, 32.0 );
    auto* multiGaugeTickWidthSlider = makeMultiGaugeMetricSlider( multiGauge->tickWidth(), 0.5, 12.0 );
    auto* multiGaugeMajorTickLengthSlider = makeMultiGaugeMetricSlider( multiGauge->majorTickLength(),
                                                                        0.0,
                                                                        32.0 );
    auto* multiGaugeMajorTickWidthSlider = makeMultiGaugeMetricSlider( multiGauge->majorTickWidth(),
                                                                       0.5,
                                                                       12.0 );
    auto* multiGaugeTickPaddingSlider = makeMultiGaugeMetricSlider( multiGauge->tickPadding(), 0.0, 40.0 );
    auto* multiGaugeTickColorButton = new ExColorPickerButton( multiGaugePropertyPage );
    multiGaugeTickColorButton->setSelectedColor(
        multiGauge->palette().color( QPalette::Active, QPalette::Text ) );
    auto* multiGaugeLabelsVisibleCheck = new QCheckBox( tr( "显示刻度标签" ), multiGaugePropertyPage );
    multiGaugeLabelsVisibleCheck->setChecked( multiGauge->areLabelsVisible() );
    auto* multiGaugeLabelPaddingSlider = makeMultiGaugeMetricSlider( multiGauge->labelPadding(),
                                                                     0.0,
                                                                     40.0 );
    auto* multiGaugeLabelFontSizeSlider = makeValueSlider( multiGaugePropertyPage,
                                                           1,
                                                           48,
                                                           multiGauge->labelFontPixelSize() );
    auto* multiGaugeLabelColorButton = new ExColorPickerButton( multiGaugePropertyPage );
    multiGaugeLabelColorButton->setSelectedColor(
        multiGauge->palette().color( QPalette::Active, QPalette::Text ) );

    multiGaugeScaleForm->addRow( tr( "主刻度数量" ), multiGaugeMajorTickCountSlider );
    multiGaugeScaleForm->addRow( tr( "每段次刻度数量" ), multiGaugeMinorTickCountSlider );
    multiGaugeScaleForm->addRow( tr( "次刻度长度" ), multiGaugeTickLengthSlider );
    multiGaugeScaleForm->addRow( tr( "次刻度宽度" ), multiGaugeTickWidthSlider );
    multiGaugeScaleForm->addRow( tr( "主刻度长度" ), multiGaugeMajorTickLengthSlider );
    multiGaugeScaleForm->addRow( tr( "主刻度宽度" ), multiGaugeMajorTickWidthSlider );
    multiGaugeScaleForm->addRow( tr( "刻度内边距" ), multiGaugeTickPaddingSlider );
    multiGaugeScaleForm->addRow( tr( "刻度颜色" ), multiGaugeTickColorButton );
    multiGaugeScaleForm->addRow( multiGaugeLabelsVisibleCheck );
    multiGaugeScaleForm->addRow( tr( "标签内边距" ), multiGaugeLabelPaddingSlider );
    multiGaugeScaleForm->addRow( tr( "标签字号" ), multiGaugeLabelFontSizeSlider );
    multiGaugeScaleForm->addRow( tr( "标签颜色" ), multiGaugeLabelColorButton );

    auto* multiGaugeNeedleStyleCombo = new ExComboBox( multiGaugePropertyPage );
    multiGaugeNeedleStyleCombo->addItem( tr( "无指针" ), ExMultiRadialGauge::NoNeedle );
    multiGaugeNeedleStyleCombo->addItem( tr( "线形指针" ), ExMultiRadialGauge::LineNeedle );
    multiGaugeNeedleStyleCombo->addItem( tr( "三角指针" ), ExMultiRadialGauge::TriangleNeedle );
    multiGaugeNeedleStyleCombo->setCurrentIndex(
        multiGaugeNeedleStyleCombo->findData( multiGauge->needleStyle() ) );
    auto* multiGaugeNeedleWidthSlider = makeMultiGaugeMetricSlider( multiGauge->needleWidth(), 0.5, 32.0 );
    auto* multiGaugeNeedleLengthSlider = makeValueSlider( multiGaugePropertyPage,
                                                          5,
                                                          120,
                                                          qRound( multiGauge->needleLength() * 100.0 ),
                                                          1,
                                                          5,
                                                          100,
                                                          2 );
    auto* multiGaugeNeedleOffsetXSlider = makeValueSlider( multiGaugePropertyPage,
                                                           -100,
                                                           100,
                                                           qRound( multiGauge->needleOffset().x() * 100.0 ),
                                                           1,
                                                           5,
                                                           100,
                                                           2 );
    auto* multiGaugeNeedleOffsetYSlider = makeValueSlider( multiGaugePropertyPage,
                                                           -100,
                                                           100,
                                                           qRound( multiGauge->needleOffset().y() * 100.0 ),
                                                           1,
                                                           5,
                                                           100,
                                                           2 );
    auto* multiGaugeHubVisibleCheck = new QCheckBox( tr( "显示公共轴心" ), multiGaugePropertyPage );
    multiGaugeHubVisibleCheck->setChecked( multiGauge->isHubVisible() );
    auto* multiGaugeHubRadiusSlider = makeMultiGaugeMetricSlider( multiGauge->hubRadius(), 0.0, 32.0 );
    auto* multiGaugeHubColorButton = new ExColorPickerButton( multiGaugePropertyPage );
    multiGaugeHubColorButton->setSelectedColor( multiGauge->hubColor() );

    multiGaugeNeedleForm->addRow( tr( "指针样式" ), multiGaugeNeedleStyleCombo );
    multiGaugeNeedleForm->addRow( tr( "指针宽度" ), multiGaugeNeedleWidthSlider );
    multiGaugeNeedleForm->addRow( tr( "指针长度比例" ), multiGaugeNeedleLengthSlider );
    multiGaugeNeedleForm->addRow( tr( "指针中心 X" ), multiGaugeNeedleOffsetXSlider );
    multiGaugeNeedleForm->addRow( tr( "指针中心 Y" ), multiGaugeNeedleOffsetYSlider );
    multiGaugeNeedleForm->addRow( multiGaugeHubVisibleCheck );
    multiGaugeNeedleForm->addRow( tr( "轴心半径" ), multiGaugeHubRadiusSlider );
    multiGaugeNeedleForm->addRow( tr( "轴心颜色" ), multiGaugeHubColorButton );

    const QList<ExMultiRadialGaugeItem*> multiGaugeItems{
        multiGaugeGoodItem,
        multiGaugeBetterItem,
        multiGaugePerfectItem
    };
    QList<QSlider*> multiGaugeItemValueSliders;
    for ( int index = 0; index < multiGaugeItems.size(); ++index )
    {
        ExMultiRadialGaugeItem* item = multiGaugeItems.at( index );
        const QString itemName = tr( "项目 %1" ).arg( index + 1 );
        auto* visibleCheck = new QCheckBox( tr( "显示" ), multiGaugePropertyPage );
        visibleCheck->setChecked( item->isVisible() );
        auto* labelEdit = new QLineEdit( item->label(), multiGaugePropertyPage );
        auto* itemValueSlider = makeValueSlider( multiGaugePropertyPage,
                                                 qRound( multiGauge->minimum() ),
                                                 qRound( multiGauge->maximum() ),
                                                 qRound( item->value() ) );
        auto* colorButton = new ExColorPickerButton( multiGaugePropertyPage );
        colorButton->setSelectedColor( item->color() );
        multiGaugeItemValueSliders.append( itemValueSlider );
        multiGaugeDataForm->addRow( itemName + tr( " 可见" ), visibleCheck );
        multiGaugeDataForm->addRow( itemName + tr( " 名称" ), labelEdit );
        multiGaugeDataForm->addRow( itemName + tr( " 数值" ), itemValueSlider );
        multiGaugeDataForm->addRow( itemName + tr( " 颜色" ), colorButton );
        connect( visibleCheck, &QCheckBox::toggled, item, &ExMultiRadialGaugeItem::setVisible );
        connect( labelEdit, &QLineEdit::textChanged, item, &ExMultiRadialGaugeItem::setLabel );
        connectScaledSlider( itemValueSlider, item, 1.0, &ExMultiRadialGaugeItem::setValue );
        connect( colorButton,
                 &ExColorPickerButton::selectedColorChanged,
                 item,
                 &ExMultiRadialGaugeItem::setColor );

        const auto addOffsetRow = [&]( const QString& label,
                                       qreal value,
                                       bool titleOffset,
                                       bool xCoordinate )
        {
            auto* slider = makeValueSlider( multiGaugePropertyPage,
                                            -120,
                                            120,
                                            qRound( value * 100.0 ),
                                            1,
                                            5,
                                            100,
                                            2 );
            multiGaugePositionForm->addRow( itemName + label, slider );
            connect( slider, &QSlider::valueChanged, item, [=]( int sliderValue )
                     {
                         QPointF offset = titleOffset ? item->titleOffset() : item->detailOffset();
                         if ( xCoordinate )
                         {
                             offset.setX( sliderValue / 100.0 );
                         }
                         else
                         {
                             offset.setY( sliderValue / 100.0 );
                         }
                         if ( titleOffset )
                         {
                             item->setTitleOffset( offset );
                         }
                         else
                         {
                             item->setDetailOffset( offset );
                         }
                     } );
        };
        addOffsetRow( tr( " 标题 X" ), item->titleOffset().x(), true, true );
        addOffsetRow( tr( " 标题 Y" ), item->titleOffset().y(), true, false );
        addOffsetRow( tr( " 数值 X" ), item->detailOffset().x(), false, true );
        addOffsetRow( tr( " 数值 Y" ), item->detailOffset().y(), false, false );
    }

    auto* multiGaugeTitleVisibleCheck = new QCheckBox( tr( "显示名称" ), multiGaugePropertyPage );
    multiGaugeTitleVisibleCheck->setChecked( multiGauge->isTitleVisible() );
    auto* multiGaugeDetailVisibleCheck = new QCheckBox( tr( "显示数值" ), multiGaugePropertyPage );
    multiGaugeDetailVisibleCheck->setChecked( multiGauge->isDetailVisible() );
    auto* multiGaugeDetailBadgeVisibleCheck = new QCheckBox( tr( "数值使用实心徽标" ),
                                                             multiGaugePropertyPage );
    multiGaugeDetailBadgeVisibleCheck->setChecked( multiGauge->isDetailBadgeVisible() );
    auto* multiGaugeTitleFontSizeSlider = makeValueSlider( multiGaugePropertyPage,
                                                           1,
                                                           48,
                                                           multiGauge->titleFontPixelSize() );
    auto* multiGaugeDetailFontSizeSlider = makeValueSlider( multiGaugePropertyPage,
                                                            1,
                                                            48,
                                                            multiGauge->detailFontPixelSize() );
    auto* multiGaugeTitleColorButton = new ExColorPickerButton( multiGaugePropertyPage );
    multiGaugeTitleColorButton->setSelectedColor(
        multiGauge->palette().color( QPalette::Active, QPalette::Text ) );
    auto* multiGaugeDetailTextColorButton = new ExColorPickerButton( multiGaugePropertyPage );
    multiGaugeDetailTextColorButton->setSelectedColor( QColor( Qt::white ) );
    auto* multiGaugeDetailBadgePaddingSlider = makeMultiGaugeMetricSlider(
        multiGauge->detailBadgePadding(),
        0.0,
        32.0 );
    auto* multiGaugeValueSuffixEdit = new QLineEdit( multiGauge->valueSuffix(), multiGaugePropertyPage );
    auto* multiGaugeValueDecimalsSlider = makeValueSlider( multiGaugePropertyPage,
                                                           0,
                                                           6,
                                                           multiGauge->valueDecimals() );

    multiGaugeTextForm->addRow( multiGaugeTitleVisibleCheck );
    multiGaugeTextForm->addRow( multiGaugeDetailVisibleCheck );
    multiGaugeTextForm->addRow( multiGaugeDetailBadgeVisibleCheck );
    multiGaugeTextForm->addRow( tr( "名称字号" ), multiGaugeTitleFontSizeSlider );
    multiGaugeTextForm->addRow( tr( "数值字号" ), multiGaugeDetailFontSizeSlider );
    multiGaugeTextForm->addRow( tr( "名称颜色" ), multiGaugeTitleColorButton );
    multiGaugeTextForm->addRow( tr( "数值文字颜色" ), multiGaugeDetailTextColorButton );
    multiGaugeTextForm->addRow( tr( "徽标水平内边距" ), multiGaugeDetailBadgePaddingSlider );
    multiGaugeTextForm->addRow( tr( "数值后缀" ), multiGaugeValueSuffixEdit );
    multiGaugeTextForm->addRow( tr( "数值小数位" ), multiGaugeValueDecimalsSlider );

    connect( multiGaugeMinimumSlider, &QSlider::valueChanged, multiGauge, [=]( int minimum )
             {
                 if ( minimum >= multiGaugeMaximumSlider->value() )
                 {
                     multiGaugeMaximumSlider->setValue( minimum + 1 );
                 }
                 multiGauge->setMinimum( minimum );
                 for ( QSlider* slider : multiGaugeItemValueSliders )
                 {
                     slider->setMinimum( minimum );
                 }
             } );
    connect( multiGaugeMaximumSlider, &QSlider::valueChanged, multiGauge, [=]( int maximum )
             {
                 if ( maximum <= multiGaugeMinimumSlider->value() )
                 {
                     multiGaugeMinimumSlider->setValue( maximum - 1 );
                 }
                 multiGauge->setMaximum( maximum );
                 for ( QSlider* slider : multiGaugeItemValueSliders )
                 {
                     slider->setMaximum( maximum );
                 }
             } );
    connectScaledSlider( multiGaugeMinimumAngleSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setMinimumAngle );
    connectScaledSlider( multiGaugeMaximumAngleSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setMaximumAngle );
    connectScaledSlider( multiGaugeScalePaddingSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setScalePadding );
    connect( multiGaugeTrackVisibleCheck,
             &QCheckBox::toggled,
             multiGauge,
             &ExMultiRadialGauge::setTrackVisible );
    connectScaledSlider( multiGaugeTrackWidthSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setTrackWidth );
    connect( multiGaugeTrackColorButton,
             &ExColorPickerButton::selectedColorChanged,
             multiGauge,
             &ExMultiRadialGauge::setTrackColor );
    connect( multiGaugeTrackCapStyleCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             multiGauge,
             [=]( int )
             {
                 multiGauge->setTrackCapStyle(
                     static_cast<Qt::PenCapStyle>( multiGaugeTrackCapStyleCombo->currentData().toInt() ) );
             } );
    connect( multiGaugeProgressVisibleCheck,
             &QCheckBox::toggled,
             multiGauge,
             &ExMultiRadialGauge::setProgressVisible );
    connect( multiGaugeProgressOverlapCheck,
             &QCheckBox::toggled,
             multiGauge,
             &ExMultiRadialGauge::setProgressOverlap );
    connect( multiGaugeProgressOverlapCheck,
             &QCheckBox::toggled,
             multiGaugeProgressSpacingSlider,
             [multiGaugeProgressSpacingSlider]( bool overlap )
             {
                 multiGaugeProgressSpacingSlider->setEnabled( !overlap );
             } );
    connectScaledSlider( multiGaugeProgressWidthSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setProgressWidth );
    connectScaledSlider( multiGaugeProgressSpacingSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setProgressSpacing );
    connect( multiGaugeProgressCapStyleCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             multiGauge,
             [=]( int )
             {
                 multiGauge->setProgressCapStyle(
                     static_cast<Qt::PenCapStyle>( multiGaugeProgressCapStyleCombo->currentData().toInt() ) );
             } );
    connectScaledSlider( multiGaugeAnimationSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setValueAnimationDuration );

    connectScaledSlider( multiGaugeMajorTickCountSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setMajorTickCount );
    connectScaledSlider( multiGaugeMinorTickCountSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setMinorTickCount );
    connectScaledSlider( multiGaugeTickLengthSlider, multiGauge, 2.0, &ExMultiRadialGauge::setTickLength );
    connectScaledSlider( multiGaugeTickWidthSlider, multiGauge, 2.0, &ExMultiRadialGauge::setTickWidth );
    connectScaledSlider( multiGaugeMajorTickLengthSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setMajorTickLength );
    connectScaledSlider( multiGaugeMajorTickWidthSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setMajorTickWidth );
    connectScaledSlider( multiGaugeTickPaddingSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setTickPadding );
    connect( multiGaugeTickColorButton,
             &ExColorPickerButton::selectedColorChanged,
             multiGauge,
             &ExMultiRadialGauge::setTickColor );
    connect( multiGaugeLabelsVisibleCheck,
             &QCheckBox::toggled,
             multiGauge,
             &ExMultiRadialGauge::setLabelsVisible );
    connectScaledSlider( multiGaugeLabelPaddingSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setLabelPadding );
    connectScaledSlider( multiGaugeLabelFontSizeSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setLabelFontPixelSize );
    connect( multiGaugeLabelColorButton,
             &ExColorPickerButton::selectedColorChanged,
             multiGauge,
             &ExMultiRadialGauge::setLabelColor );

    connect( multiGaugeNeedleStyleCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             multiGauge,
             [=]( int )
             {
                 multiGauge->setNeedleStyle(
                     static_cast<ExMultiRadialGauge::NeedleStyle>(
                         multiGaugeNeedleStyleCombo->currentData().toInt() ) );
             } );
    connectScaledSlider( multiGaugeNeedleWidthSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setNeedleWidth );
    connectScaledSlider( multiGaugeNeedleLengthSlider,
                         multiGauge,
                         100.0,
                         &ExMultiRadialGauge::setNeedleLength );
    const auto updateMultiGaugeNeedleOffset = [=]
    {
        multiGauge->setNeedleOffset( QPointF( multiGaugeNeedleOffsetXSlider->value() / 100.0,
                                              multiGaugeNeedleOffsetYSlider->value() / 100.0 ) );
    };
    connect( multiGaugeNeedleOffsetXSlider,
             &QSlider::valueChanged,
             multiGauge,
             updateMultiGaugeNeedleOffset );
    connect( multiGaugeNeedleOffsetYSlider,
             &QSlider::valueChanged,
             multiGauge,
             updateMultiGaugeNeedleOffset );
    connect( multiGaugeHubVisibleCheck,
             &QCheckBox::toggled,
             multiGauge,
             &ExMultiRadialGauge::setHubVisible );
    connectScaledSlider( multiGaugeHubRadiusSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setHubRadius );
    connect( multiGaugeHubColorButton,
             &ExColorPickerButton::selectedColorChanged,
             multiGauge,
             &ExMultiRadialGauge::setHubColor );

    connect( multiGaugeTitleVisibleCheck,
             &QCheckBox::toggled,
             multiGauge,
             &ExMultiRadialGauge::setTitleVisible );
    connect( multiGaugeDetailVisibleCheck,
             &QCheckBox::toggled,
             multiGauge,
             &ExMultiRadialGauge::setDetailVisible );
    connect( multiGaugeDetailBadgeVisibleCheck,
             &QCheckBox::toggled,
             multiGauge,
             &ExMultiRadialGauge::setDetailBadgeVisible );
    connectScaledSlider( multiGaugeTitleFontSizeSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setTitleFontPixelSize );
    connectScaledSlider( multiGaugeDetailFontSizeSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setDetailFontPixelSize );
    connect( multiGaugeTitleColorButton,
             &ExColorPickerButton::selectedColorChanged,
             multiGauge,
             &ExMultiRadialGauge::setTitleColor );
    connect( multiGaugeDetailTextColorButton,
             &ExColorPickerButton::selectedColorChanged,
             multiGauge,
             &ExMultiRadialGauge::setDetailTextColor );
    connectScaledSlider( multiGaugeDetailBadgePaddingSlider,
                         multiGauge,
                         2.0,
                         &ExMultiRadialGauge::setDetailBadgePadding );
    connect( multiGaugeValueSuffixEdit,
             &QLineEdit::textChanged,
             multiGauge,
             &ExMultiRadialGauge::setValueSuffix );
    connectScaledSlider( multiGaugeValueDecimalsSlider,
                         multiGauge,
                         1.0,
                         &ExMultiRadialGauge::setValueDecimals );

    multiGaugePreviewLayout->addWidget( multiGaugeEditorTabs, 1 );
    multiGaugePropertyLayout->addLayout( multiGaugePreviewLayout );
    auto* multiGaugeHint = new QLabel(
        tr( "共享范围、刻度、Track 和轴心；每个数据项分别配置名称、数值、颜色以及标题和详情位置。关闭重叠后，进度弧会改为同心排列。" ),
        multiGaugePropertyPage );
    multiGaugeHint->setWordWrap( true );
    multiGaugePropertyLayout->addWidget( multiGaugeHint );
    multiGaugePropertyLayout->addStretch();

    mainLayout->addWidget( propertiesCard );
    mainLayout->addStretch();
    scrollArea->setWidget( content );
}
