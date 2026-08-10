#include "audiolevelmetershowcasewidget.h"

#include <exaudiolevelmeter.h>
#include <excolorpickerbutton.h>
#include <excombobox.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

#include <cmath>

namespace
{

QWidget* makeCard( QWidget* parent )
{
    auto* card = new QWidget( parent );
    card->setProperty( "isCard", true );
    card->setAttribute( Qt::WA_StyledBackground, true );
    return card;
}

QLabel* makeTitle( const QString& text, QWidget* parent )
{
    auto* label = new QLabel( text, parent );
    QFont font = label->font();
    font.setBold( true );
    font.setPixelSize( 14 );
    label->setFont( font );
    return label;
}

void useReferenceColors( ExAudioLevelMeter* meter )
{
    meter->setBackgroundColor( QColor( QStringLiteral( "#111111" ) ) );
    meter->setActiveColor( QColor( QStringLiteral( "#FF8C00" ) ) );
    meter->setInactiveColor( QColor( QStringLiteral( "#292929" ) ) );
    meter->setScaleColor( QColor( QStringLiteral( "#5A5A5A" ) ) );
    meter->setPeakColor( QColor( QStringLiteral( "#F5F5F5" ) ) );
}

} // namespace

AudioLevelMeterShowcaseWidget::AudioLevelMeterShowcaseWidget( QWidget* parent )
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
    auto* mainLayout = new QVBoxLayout( content );
    mainLayout->setContentsMargins( 16, 16, 16, 16 );
    mainLayout->setSpacing( 16 );

    auto* pageTitle = new QLabel( tr( "ExAudioLevelMeter" ), content );
    QFont pageTitleFont = pageTitle->font();
    pageTitleFont.setPointSize( 16 );
    pageTitleFont.setBold( true );
    pageTitle->setFont( pageTitleFont );
    mainLayout->addWidget( pageTitle );

    auto* description = new QLabel(
        tr( "只读的实时音频电平表。接收每个声道的 dBFS 或线性幅度，支持单声道、立体声、峰值保持、衰减和超时归零。" ),
        content );
    description->setWordWrap( true );
    mainLayout->addWidget( description );

    auto* examplesCard = makeCard( content );
    auto* examplesLayout = new QVBoxLayout( examplesCard );
    examplesLayout->setContentsMargins( 16, 16, 16, 16 );
    examplesLayout->setSpacing( 12 );
    examplesLayout->addWidget( makeTitle( tr( "单声道与立体声" ), examplesCard ) );

    auto* metersRow = new QHBoxLayout;
    metersRow->setSpacing( 24 );
    metersRow->addStretch();

    auto* monoColumn = new QVBoxLayout;
    auto* monoLabel = new QLabel( tr( "Mono" ), examplesCard );
    monoLabel->setAlignment( Qt::AlignCenter );
    auto* monoMeter = new ExAudioLevelMeter( examplesCard );
    monoMeter->setChannelCount( 1 );
    monoMeter->setScalePosition( ExAudioLevelMeter::RightScale );
    monoMeter->setChannelLabelsVisible( false );
    monoMeter->setFixedSize( 105, 330 );
    useReferenceColors( monoMeter );
    monoColumn->addWidget( monoLabel );
    monoColumn->addWidget( monoMeter );
    metersRow->addLayout( monoColumn );

    auto* stereoColumn = new QVBoxLayout;
    auto* stereoLabel = new QLabel( tr( "Stereo" ), examplesCard );
    stereoLabel->setAlignment( Qt::AlignCenter );
    auto* stereoMeter = new ExAudioLevelMeter( examplesCard );
    stereoMeter->setScalePosition( ExAudioLevelMeter::CenterScale );
    stereoMeter->setFixedSize( 190, 330 );
    useReferenceColors( stereoMeter );
    stereoColumn->addWidget( stereoLabel );
    stereoColumn->addWidget( stereoMeter );
    metersRow->addLayout( stereoColumn );
    metersRow->addStretch();
    examplesLayout->addLayout( metersRow );
    mainLayout->addWidget( examplesCard );

    auto* propertiesCard = makeCard( content );
    auto* propertiesLayout = new QVBoxLayout( propertiesCard );
    propertiesLayout->setContentsMargins( 16, 16, 16, 16 );
    propertiesLayout->setSpacing( 12 );
    propertiesLayout->addWidget( makeTitle( tr( "实时属性" ), propertiesCard ) );

    auto* editorLayout = new QHBoxLayout;
    editorLayout->setSpacing( 28 );
    auto* preview = new ExAudioLevelMeter( propertiesCard );
    preview->setScalePosition( ExAudioLevelMeter::CenterScale );
    preview->setCustomScaleValues( QVector<qreal>{0.0, -3.0, -6.0, -12.0, -24.0, -36.0, -48.0, -60.0} );
    preview->setMinimumSize( 220, 390 );
    preview->setMaximumWidth( 300 );
    useReferenceColors( preview );
    editorLayout->addWidget( preview, 1, Qt::AlignHCenter | Qt::AlignTop );

    auto* editor = new QWidget( propertiesCard );
    auto* form = new QFormLayout( editor );
    form->setContentsMargins( 0, 0, 0, 0 );
    form->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
    form->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );

    auto* channelsSpin = new QSpinBox( editor );
    channelsSpin->setRange( 1, 8 );
    channelsSpin->setValue( preview->channelCount() );

    auto* scaleCombo = new ExComboBox( editor );
    scaleCombo->addItem( tr( "隐藏" ), ExAudioLevelMeter::NoScale );
    scaleCombo->addItem( tr( "左侧" ), ExAudioLevelMeter::LeftScale );
    scaleCombo->addItem( tr( "右侧" ), ExAudioLevelMeter::RightScale );
    scaleCombo->addItem( tr( "中间（立体声）" ), ExAudioLevelMeter::CenterScale );
    scaleCombo->setCurrentIndex( scaleCombo->findData( preview->scalePosition() ) );

    auto* scaleModeCombo = new ExComboBox( editor );
    scaleModeCombo->addItem( tr( "按间隔" ), ExAudioLevelMeter::IntervalScale );
    scaleModeCombo->addItem( tr( "固定数量" ), ExAudioLevelMeter::FixedTickCount );
    scaleModeCombo->addItem( tr( "自定义数值" ), ExAudioLevelMeter::CustomScale );

    auto* tickCountSpin = new QSpinBox( editor );
    tickCountSpin->setRange( 2, 64 );
    tickCountSpin->setValue( preview->scaleTickCount() );

    auto* precisionSpin = new QSpinBox( editor );
    precisionSpin->setRange( 0, 3 );
    precisionSpin->setValue( preview->scalePrecision() );

    auto* unitEdit = new QLineEdit( preview->scaleUnit(), editor );
    auto* unitCheck = new QCheckBox( tr( "在刻度后显示单位" ), editor );
    unitCheck->setChecked( preview->isScaleUnitVisible() );
    auto* tickMarksCheck = new QCheckBox( tr( "显示短刻度线" ), editor );
    tickMarksCheck->setChecked( preview->areScaleTickMarksVisible() );

    auto* colorModeCombo = new ExComboBox( editor );
    colorModeCombo->addItem( tr( "统一颜色" ), ExAudioLevelMeter::SingleColor );
    colorModeCombo->addItem( tr( "警告/过载分区" ), ExAudioLevelMeter::ThresholdColors );
    colorModeCombo->addItem( tr( "连续渐变" ), ExAudioLevelMeter::GradientColors );

    auto* segmentSpin = new QSpinBox( editor );
    segmentSpin->setRange( 2, 120 );
    segmentSpin->setValue( preview->segmentCount() );

    auto* minimumSpin = new QDoubleSpinBox( editor );
    minimumSpin->setRange( -160.0, -1.0 );
    minimumSpin->setSuffix( QStringLiteral( " dB" ) );
    minimumSpin->setValue( preview->minimumDecibels() );

    auto* warningSpin = new QDoubleSpinBox( editor );
    warningSpin->setRange( -160.0, 24.0 );
    warningSpin->setSuffix( QStringLiteral( " dB" ) );
    warningSpin->setValue( preview->warningDecibels() );

    auto* clipSpin = new QDoubleSpinBox( editor );
    clipSpin->setRange( -160.0, 24.0 );
    clipSpin->setSuffix( QStringLiteral( " dB" ) );
    clipSpin->setValue( preview->clipDecibels() );

    auto* decaySpin = new QDoubleSpinBox( editor );
    decaySpin->setRange( 0.0, 200.0 );
    decaySpin->setSuffix( tr( " dB/s" ) );
    decaySpin->setValue( preview->decayRate() );

    auto* holdSpin = new QSpinBox( editor );
    holdSpin->setRange( 0, 10000 );
    holdSpin->setSuffix( QStringLiteral( " ms" ) );
    holdSpin->setValue( preview->peakHoldDuration() );

    auto* peakCheck = new QCheckBox( tr( "显示峰值保持" ), editor );
    peakCheck->setChecked( preview->isPeakHoldEnabled() );
    auto* labelsCheck = new QCheckBox( tr( "显示声道标签" ), editor );
    labelsCheck->setChecked( preview->areChannelLabelsVisible() );

    auto* activeColorButton = new ExColorPickerButton( editor );
    activeColorButton->setSelectedColor( preview->activeColor() );
    auto* inactiveColorButton = new ExColorPickerButton( editor );
    inactiveColorButton->setSelectedColor( preview->inactiveColor() );
    auto* backgroundColorButton = new ExColorPickerButton( editor );
    backgroundColorButton->setSelectedColor( preview->backgroundColor() );
    auto* warningColorButton = new ExColorPickerButton( editor );
    warningColorButton->setSelectedColor( preview->warningColor() );
    auto* clipColorButton = new ExColorPickerButton( editor );
    clipColorButton->setSelectedColor( preview->clipColor() );

    form->addRow( tr( "声道数" ), channelsSpin );
    form->addRow( tr( "dB 刻度" ), scaleCombo );
    form->addRow( tr( "刻度生成" ), scaleModeCombo );
    form->addRow( tr( "刻度数量" ), tickCountSpin );
    form->addRow( tr( "小数位数" ), precisionSpin );
    form->addRow( tr( "刻度单位" ), unitEdit );
    form->addRow( tr( "颜色模式" ), colorModeCombo );
    form->addRow( tr( "分段数量" ), segmentSpin );
    form->addRow( tr( "最小电平" ), minimumSpin );
    form->addRow( tr( "警告电平" ), warningSpin );
    form->addRow( tr( "过载电平" ), clipSpin );
    form->addRow( tr( "衰减速度" ), decaySpin );
    form->addRow( tr( "峰值保持" ), holdSpin );
    form->addRow( tr( "激活颜色" ), activeColorButton );
    form->addRow( tr( "未激活颜色" ), inactiveColorButton );
    form->addRow( tr( "背景颜色" ), backgroundColorButton );
    form->addRow( tr( "警告颜色" ), warningColorButton );
    form->addRow( tr( "过载颜色" ), clipColorButton );
    form->addRow( peakCheck );
    form->addRow( labelsCheck );
    form->addRow( unitCheck );
    form->addRow( tickMarksCheck );
    editorLayout->addWidget( editor, 1 );
    propertiesLayout->addLayout( editorLayout );
    mainLayout->addWidget( propertiesCard );
    mainLayout->addStretch();

    connect( channelsSpin,
             QOverload<int>::of( &QSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setChannelCount );
    connect( scaleCombo,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             preview,
             [preview, scaleCombo]( int index )
             {
                 preview->setScalePosition(
                     static_cast<ExAudioLevelMeter::ScalePosition>( scaleCombo->itemData( index ).toInt() ) );
             } );
    connect( scaleModeCombo,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             preview,
             [preview, scaleModeCombo]( int index )
             {
                 preview->setScaleMode(
                     static_cast<ExAudioLevelMeter::ScaleMode>( scaleModeCombo->itemData( index ).toInt() ) );
             } );
    connect( tickCountSpin,
             QOverload<int>::of( &QSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setScaleTickCount );
    connect( precisionSpin,
             QOverload<int>::of( &QSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setScalePrecision );
    connect( unitEdit, &QLineEdit::textChanged, preview, &ExAudioLevelMeter::setScaleUnit );
    connect( colorModeCombo,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             preview,
             [preview, colorModeCombo]( int index )
             {
                 preview->setColorMode(
                     static_cast<ExAudioLevelMeter::ColorMode>( colorModeCombo->itemData( index ).toInt() ) );
             } );
    connect( segmentSpin,
             QOverload<int>::of( &QSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setSegmentCount );
    connect( minimumSpin,
             QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setMinimumDecibels );
    connect( warningSpin,
             QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setWarningDecibels );
    connect( preview,
             &ExAudioLevelMeter::warningDecibelsChanged,
             warningSpin,
             &QDoubleSpinBox::setValue );
    connect( clipSpin,
             QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setClipDecibels );
    connect( preview,
             &ExAudioLevelMeter::clipDecibelsChanged,
             clipSpin,
             &QDoubleSpinBox::setValue );
    connect( decaySpin,
             QOverload<double>::of( &QDoubleSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setDecayRate );
    connect( holdSpin,
             QOverload<int>::of( &QSpinBox::valueChanged ),
             preview,
             &ExAudioLevelMeter::setPeakHoldDuration );
    connect( peakCheck, &QCheckBox::toggled, preview, &ExAudioLevelMeter::setPeakHoldEnabled );
    connect( labelsCheck, &QCheckBox::toggled, preview, &ExAudioLevelMeter::setChannelLabelsVisible );
    connect( unitCheck, &QCheckBox::toggled, preview, &ExAudioLevelMeter::setScaleUnitVisible );
    connect( tickMarksCheck,
             &QCheckBox::toggled,
             preview,
             &ExAudioLevelMeter::setScaleTickMarksVisible );
    connect( activeColorButton,
             &ExColorPickerButton::selectedColorChanged,
             preview,
             &ExAudioLevelMeter::setActiveColor );
    connect( inactiveColorButton,
             &ExColorPickerButton::selectedColorChanged,
             preview,
             &ExAudioLevelMeter::setInactiveColor );
    connect( backgroundColorButton,
             &ExColorPickerButton::selectedColorChanged,
             preview,
             &ExAudioLevelMeter::setBackgroundColor );
    connect( warningColorButton,
             &ExColorPickerButton::selectedColorChanged,
             preview,
             &ExAudioLevelMeter::setWarningColor );
    connect( clipColorButton,
             &ExColorPickerButton::selectedColorChanged,
             preview,
             &ExAudioLevelMeter::setClipColor );

    auto* animationTimer = new QTimer( this );
    animationTimer->setInterval( 45 );
    connect( animationTimer,
             &QTimer::timeout,
             this,
             [monoMeter, stereoMeter, preview, phase = qreal( 0.0 )]() mutable
             {
                 phase += 0.16;
                 const qreal noise1 = QRandomGenerator::global()->generateDouble() * 5.0;
                 const qreal noise2 = QRandomGenerator::global()->generateDouble() * 7.0;
                 const qreal left = qBound( -60.0,
                                            -35.0 + 29.0 * ( 0.5 + 0.5 * std::sin( phase ) ) + noise1,
                                            -0.5 );
                 const qreal right = qBound( -60.0,
                                             -38.0 + 31.0 * ( 0.5 + 0.5 * std::sin( phase * 0.83 + 1.2 ) )
                                                 + noise2,
                                             -0.5 );
                 monoMeter->setLevel( left );
                 stereoMeter->setStereoLevels( left, right );

                 QVector<qreal> previewLevels;
                 previewLevels.reserve( preview->channelCount() );
                 for ( int channel = 0; channel < preview->channelCount(); ++channel )
                 {
                     const qreal offset = channel * 0.71;
                     previewLevels.append( qBound( -60.0,
                                                   -40.0
                                                       + 34.0
                                                             * ( 0.5
                                                                 + 0.5
                                                                       * std::sin( phase * ( 0.75 + channel * 0.04 )
                                                                                   + offset ) )
                                                       + QRandomGenerator::global()->generateDouble() * 4.0,
                                                   -0.5 ) );
                 }
                 preview->setLevels( previewLevels );
             } );
    animationTimer->start();

    scrollArea->setWidget( content );
}
