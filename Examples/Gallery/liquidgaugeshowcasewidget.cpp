#include "liquidgaugeshowcasewidget.h"

#include "fluentui3styleproperties.h"

#include <excolorpickerbutton.h>
#include <excombobox.h>
#include <exliquidgauge.h>
#include <extabwidget.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabBar>
#include <QVBoxLayout>

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
    slider->setProperty( "showValueTip", true );
    if ( scale != 1 )
    {
        slider->setProperty( "scale", scale );
    }
    if ( precision > 0 )
    {
        slider->setProperty( "precision", precision );
    }
    return slider;
}

QColor paletteAccentColor( const QWidget* widget )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return widget->palette().color( QPalette::Accent );
#else
    return widget->palette().color( QPalette::Highlight );
#endif
}

QWidget* makePropertyPage( ExTabWidget* tabWidget, QFormLayout*& form )
{
    auto* page = new QWidget( tabWidget );
    form = new QFormLayout( page );
    form->setContentsMargins( 12, 12, 12, 12 );
    form->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    form->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
    form->setHorizontalSpacing( 12 );
    form->setVerticalSpacing( 10 );
    return page;
}

} // namespace

LiquidGaugeShowcaseWidget::LiquidGaugeShowcaseWidget( QWidget* parent )
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

    auto* title = new QLabel( tr( "ExLiquidGauge" ), content );
    QFont titleFont = title->font();
    titleFont.setPointSize( 16 );
    titleFont.setBold( true );
    title->setFont( titleFont );
    mainLayout->addWidget( title );

    auto* description = new QLabel(
        tr( "参考 Ant Design Charts Liquid 的水波图控件，继承 QProgressBar，并提供圆形、矩形、水滴和三角形裁剪、双层水波与中心文本。" ),
        content );
    description->setWordWrap( true );
    mainLayout->addWidget( description );

    auto* shapesCard = makeCard( content );
    auto* shapesLayout = new QVBoxLayout( shapesCard );
    shapesLayout->setContentsMargins( 16, 16, 16, 16 );
    shapesLayout->setSpacing( 12 );
    shapesLayout->addWidget( makeSectionTitle( tr( "内置形状" ), shapesCard ) );

    auto* samplesLayout = new QHBoxLayout;
    samplesLayout->setSpacing( 24 );
    samplesLayout->addStretch();

    struct ShapeSample
    {
        ExLiquidGauge::Shape shape;
        QString name;
        QColor color;
        int value;
    };
    const QList<ShapeSample> samples{
        {ExLiquidGauge::CircleShape, tr( "圆形" ), QColor( QStringLiteral( "#1677FF" ) ), 68},
        {ExLiquidGauge::RectShape, tr( "矩形" ), QColor( QStringLiteral( "#13C2C2" ) ), 52},
        {ExLiquidGauge::PinShape, tr( "水滴" ), QColor( QStringLiteral( "#722ED1" ) ), 76},
        {ExLiquidGauge::TriangleShape, tr( "三角形" ), QColor( QStringLiteral( "#FA8C16" ) ), 42}};
    QList<ExLiquidGauge*> sampleGauges;
    for ( const ShapeSample& sample : samples )
    {
        auto* sampleWidget = new QWidget( shapesCard );
        auto* sampleLayout = new QVBoxLayout( sampleWidget );
        sampleLayout->setContentsMargins( 0, 0, 0, 0 );
        sampleLayout->setSpacing( 8 );

        auto* liquidGauge = new ExLiquidGauge( sampleWidget );
        liquidGauge->setRange( 0, 100 );
        liquidGauge->setValue( sample.value );
        liquidGauge->setShape( sample.shape );
        liquidGauge->setWaveColor( sample.color );
        liquidGauge->setFixedSize( 150, 150 );
        sampleGauges.append( liquidGauge );

        auto* label = new QLabel( sample.name, sampleWidget );
        label->setAlignment( Qt::AlignCenter );
        sampleLayout->addWidget( liquidGauge, 0, Qt::AlignHCenter );
        sampleLayout->addWidget( label );
        samplesLayout->addWidget( sampleWidget );
    }
    samplesLayout->addStretch();
    shapesLayout->addLayout( samplesLayout );

    auto* sharedValueLayout = new QHBoxLayout;
    auto* sharedValueLabel = new QLabel( tr( "公共数值" ), shapesCard );
    auto* sharedValueSlider = makeValueSlider( shapesCard, 0, 100, 60 );
    sharedValueLayout->addWidget( sharedValueLabel );
    sharedValueLayout->addWidget( sharedValueSlider, 1 );
    shapesLayout->addLayout( sharedValueLayout );
    for ( ExLiquidGauge* sampleGauge : std::as_const( sampleGauges ) )
    {
        sampleGauge->setValue( sharedValueSlider->value() );
        connect( sharedValueSlider, &QSlider::valueChanged, sampleGauge, &QProgressBar::setValue );
    }
    mainLayout->addWidget( shapesCard );

    auto* propertiesCard = makeCard( content );
    auto* propertiesLayout = new QVBoxLayout( propertiesCard );
    propertiesLayout->setContentsMargins( 16, 16, 16, 16 );
    propertiesLayout->setSpacing( 12 );
    propertiesLayout->addWidget( makeSectionTitle( tr( "实时属性" ), propertiesCard ) );

    auto* previewLayout = new QHBoxLayout;
    previewLayout->setSpacing( 32 );

    auto* previewGauge = new ExLiquidGauge( propertiesCard );
    previewGauge->setRange( 0, 100 );
    previewGauge->setValue( 68 );
    previewGauge->setFixedSize( 260, 260 );
    previewLayout->addWidget( previewGauge, 0, Qt::AlignHCenter | Qt::AlignTop );

    auto* propertyTabs = new ExTabWidget( propertiesCard );
    propertyTabs->tabBar()->setProperty( TabBarStyleProperty, TabBarStyle::Pivot_Slide );
    propertyTabs->setMinimumWidth( 440 );
    propertyTabs->setMinimumHeight( 430 );

    QFormLayout* basicForm = nullptr;
    QFormLayout* appearanceForm = nullptr;
    auto* basicPage = makePropertyPage( propertyTabs, basicForm );
    auto* appearancePage = makePropertyPage( propertyTabs, appearanceForm );
    propertyTabs->addTab( basicPage, tr( "基础" ) );
    propertyTabs->addTab( appearancePage, tr( "外观" ) );

    auto* valueSlider = makeValueSlider( basicPage, 0, 100, previewGauge->value() );
    auto* shapeCombo = new ExComboBox( basicPage );
    shapeCombo->addItem( tr( "圆形" ), ExLiquidGauge::CircleShape );
    shapeCombo->addItem( tr( "矩形" ), ExLiquidGauge::RectShape );
    shapeCombo->addItem( tr( "水滴" ), ExLiquidGauge::PinShape );
    shapeCombo->addItem( tr( "三角形" ), ExLiquidGauge::TriangleShape );
    shapeCombo->setCurrentIndex( shapeCombo->findData( previewGauge->shape() ) );
    auto* formatEdit = new QLineEdit( previewGauge->format(), basicPage );
    auto* contentFontSlider = makeValueSlider( basicPage, 0, 72, previewGauge->contentFontPixelSize() );
    auto* animationCheck = new QCheckBox( tr( "播放水波动画" ), basicPage );
    animationCheck->setChecked( previewGauge->isAnimationEnabled() );
    auto* textVisibleCheck = new QCheckBox( tr( "显示中心文本" ), basicPage );
    textVisibleCheck->setChecked( previewGauge->isTextVisible() );
    auto* disabledCheck = new QCheckBox( tr( "禁用状态" ), basicPage );

    basicForm->addRow( tr( "数值" ), valueSlider );
    basicForm->addRow( tr( "形状" ), shapeCombo );
    basicForm->addRow( tr( "文本格式" ), formatEdit );
    basicForm->addRow( tr( "文本字号" ), contentFontSlider );
    basicForm->addRow( animationCheck );
    basicForm->addRow( textVisibleCheck );
    basicForm->addRow( disabledCheck );

    auto* amplitudeSlider = makeValueSlider( appearancePage,
                                             0,
                                             40,
                                             qRound( previewGauge->waveAmplitude() * 2.0 ),
                                             1,
                                             4,
                                             2,
                                             1 );
    auto* waveCountSlider = makeValueSlider( appearancePage, 1, 10, previewGauge->waveCount() );
    auto* durationSlider = makeValueSlider( appearancePage,
                                            200,
                                            6000,
                                            previewGauge->waveAnimationDuration(),
                                            100,
                                            500 );
    auto* secondaryOpacitySlider = makeValueSlider( appearancePage,
                                                    0,
                                                    100,
                                                    qRound( previewGauge->secondaryWaveOpacity() * 100.0 ),
                                                    1,
                                                    10,
                                                    100,
                                                    2 );
    auto* outlineWidthSlider = makeValueSlider( appearancePage,
                                                0,
                                                40,
                                                qRound( previewGauge->outlineWidth() * 2.0 ),
                                                1,
                                                4,
                                                2,
                                                1 );
    auto* outlineDistanceSlider = makeValueSlider( appearancePage,
                                                   0,
                                                   40,
                                                   qRound( previewGauge->outlineDistance() * 2.0 ),
                                                   1,
                                                   4,
                                                   2,
                                                   1 );

    auto* waveColorButton = new ExColorPickerButton( appearancePage );
    waveColorButton->setSelectedColor( paletteAccentColor( previewGauge ) );
    auto* backgroundColorButton = new ExColorPickerButton( appearancePage );
    backgroundColorButton->setSelectedColor( previewGauge->palette().color( QPalette::Base ) );
    auto* outlineColorButton = new ExColorPickerButton( appearancePage );
    outlineColorButton->setSelectedColor( paletteAccentColor( previewGauge ) );
    auto* textColorButton = new ExColorPickerButton( appearancePage );
    textColorButton->setSelectedColor( previewGauge->palette().color( QPalette::Text ) );
    auto* submergedTextColorButton = new ExColorPickerButton( appearancePage );
    submergedTextColorButton->setSelectedColor( previewGauge->palette().color( QPalette::HighlightedText ) );

    appearanceForm->addRow( tr( "波幅" ), amplitudeSlider );
    appearanceForm->addRow( tr( "波形数量" ), waveCountSlider );
    appearanceForm->addRow( tr( "动画周期" ), durationSlider );
    appearanceForm->addRow( tr( "后层水波透明度" ), secondaryOpacitySlider );
    appearanceForm->addRow( tr( "轮廓宽度" ), outlineWidthSlider );
    appearanceForm->addRow( tr( "轮廓间距" ), outlineDistanceSlider );
    appearanceForm->addRow( tr( "水波颜色" ), waveColorButton );
    appearanceForm->addRow( tr( "背景颜色" ), backgroundColorButton );
    appearanceForm->addRow( tr( "轮廓颜色" ), outlineColorButton );
    appearanceForm->addRow( tr( "液面上文字颜色" ), textColorButton );
    appearanceForm->addRow( tr( "液面下文字颜色" ), submergedTextColorButton );

    previewLayout->addWidget( propertyTabs, 1 );
    propertiesLayout->addLayout( previewLayout );

    auto* resetButton = new QPushButton( tr( "恢复默认属性" ), propertiesCard );
    propertiesLayout->addWidget( resetButton, 0, Qt::AlignRight );

    connect( valueSlider, &QSlider::valueChanged, previewGauge, &QProgressBar::setValue );
    connect( shapeCombo,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             previewGauge,
             [previewGauge, shapeCombo]( int index )
             {
                 previewGauge->setShape(
                     static_cast<ExLiquidGauge::Shape>( shapeCombo->itemData( index ).toInt() ) );
             } );
    connect( formatEdit, &QLineEdit::textChanged, previewGauge, &QProgressBar::setFormat );
    connect( contentFontSlider,
             &QSlider::valueChanged,
             previewGauge,
             &ExLiquidGauge::setContentFontPixelSize );
    connect( animationCheck, &QCheckBox::toggled, previewGauge, &ExLiquidGauge::setAnimationEnabled );
    connect( textVisibleCheck, &QCheckBox::toggled, previewGauge, &QProgressBar::setTextVisible );
    connect( disabledCheck, &QCheckBox::toggled, previewGauge, [previewGauge]( bool disabled )
             {
                 previewGauge->setEnabled( !disabled );
             } );
    connect( amplitudeSlider, &QSlider::valueChanged, previewGauge, [previewGauge]( int value )
             {
                 previewGauge->setWaveAmplitude( value / 2.0 );
             } );
    connect( waveCountSlider, &QSlider::valueChanged, previewGauge, &ExLiquidGauge::setWaveCount );
    connect( durationSlider,
             &QSlider::valueChanged,
             previewGauge,
             &ExLiquidGauge::setWaveAnimationDuration );
    connect( secondaryOpacitySlider, &QSlider::valueChanged, previewGauge, [previewGauge]( int value )
             {
                 previewGauge->setSecondaryWaveOpacity( value / 100.0 );
             } );
    connect( outlineWidthSlider, &QSlider::valueChanged, previewGauge, [previewGauge]( int value )
             {
                 previewGauge->setOutlineWidth( value / 2.0 );
             } );
    connect( outlineDistanceSlider, &QSlider::valueChanged, previewGauge, [previewGauge]( int value )
             {
                 previewGauge->setOutlineDistance( value / 2.0 );
             } );
    connect( waveColorButton,
             &ExColorPickerButton::selectedColorChanged,
             previewGauge,
             &ExLiquidGauge::setWaveColor );
    connect( backgroundColorButton,
             &ExColorPickerButton::selectedColorChanged,
             previewGauge,
             &ExLiquidGauge::setBackgroundColor );
    connect( outlineColorButton,
             &ExColorPickerButton::selectedColorChanged,
             previewGauge,
             &ExLiquidGauge::setOutlineColor );
    connect( textColorButton,
             &ExColorPickerButton::selectedColorChanged,
             previewGauge,
             &ExLiquidGauge::setTextColor );
    connect( submergedTextColorButton,
             &ExColorPickerButton::selectedColorChanged,
             previewGauge,
             &ExLiquidGauge::setSubmergedTextColor );

    connect( resetButton, &QPushButton::clicked, previewGauge, [=]
             {
                 valueSlider->setValue( 68 );
                 shapeCombo->setCurrentIndex( shapeCombo->findData( ExLiquidGauge::CircleShape ) );
                 formatEdit->setText( QStringLiteral( "%p%" ) );
                 contentFontSlider->setValue( 0 );
                 animationCheck->setChecked( true );
                 textVisibleCheck->setChecked( true );
                 disabledCheck->setChecked( false );
                 amplitudeSlider->setValue( 12 );
                 waveCountSlider->setValue( 3 );
                 durationSlider->setValue( 2400 );
                 secondaryOpacitySlider->setValue( 45 );
                 outlineWidthSlider->setValue( 4 );
                 outlineDistanceSlider->setValue( 6 );

                 const QSignalBlocker waveColorBlocker( waveColorButton );
                 const QSignalBlocker backgroundColorBlocker( backgroundColorButton );
                 const QSignalBlocker outlineColorBlocker( outlineColorButton );
                 const QSignalBlocker textColorBlocker( textColorButton );
                 const QSignalBlocker submergedTextColorBlocker( submergedTextColorButton );
                 previewGauge->setWaveColor( QColor() );
                 previewGauge->setBackgroundColor( QColor() );
                 previewGauge->setOutlineColor( QColor() );
                 previewGauge->setTextColor( QColor() );
                 previewGauge->setSubmergedTextColor( QColor() );
                 waveColorButton->setSelectedColor( paletteAccentColor( previewGauge ) );
                 backgroundColorButton->setSelectedColor( previewGauge->palette().color( QPalette::Base ) );
                 outlineColorButton->setSelectedColor( paletteAccentColor( previewGauge ) );
                 textColorButton->setSelectedColor( previewGauge->palette().color( QPalette::Text ) );
                 submergedTextColorButton->setSelectedColor(
                     previewGauge->palette().color( QPalette::HighlightedText ) );
             } );

    auto* code = new QPlainTextEdit( propertiesCard );
    code->setReadOnly( true );
    code->setMaximumHeight( 150 );
    code->setFont( QFontDatabase::systemFont( QFontDatabase::FixedFont ) );
    code->setPlainText( QStringLiteral(
        "auto *gauge = new ExLiquidGauge(this);\n"
        "gauge->setRange(0, 100);\n"
        "gauge->setValue(68);\n"
        "gauge->setShape(ExLiquidGauge::CircleShape);\n"
        "gauge->setWaveAmplitude(6.0);\n"
        "gauge->setWaveCount(3);\n"
        "gauge->setWaveAnimationDuration(2400);" ) );
    propertiesLayout->addWidget( code );

    mainLayout->addWidget( propertiesCard );
    mainLayout->addStretch();
    scrollArea->setWidget( content );
}
