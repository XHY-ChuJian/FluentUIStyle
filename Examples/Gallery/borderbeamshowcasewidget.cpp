#include "borderbeamshowcasewidget.h"

#include <exborderbeam.h>
#include <exborderbeambutton.h>
#include <excolorpickerbutton.h>
#include <excombobox.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>

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

QSlider* makeSlider( QWidget* parent, int minimum, int maximum, int value )
{
    auto* slider = new QSlider( Qt::Horizontal, parent );
    slider->setRange( minimum, maximum );
    slider->setValue( value );
    slider->setTracking( true );
    slider->setProperty( "showValueTip", true );
    return slider;
}

ExBorderBeam* makeBeamSample( QWidget* parent,
                              const QString& title,
                              const QString& description )
{
    auto* beam = new ExBorderBeam( parent );
    beam->setCornerRadius( 14.0 );
    beam->setMinimumSize( 240, 150 );

    auto* layout = new QVBoxLayout( beam );
    layout->setContentsMargins( 18, 18, 18, 18 );
    layout->setSpacing( 8 );
    layout->addStretch();

    auto* titleLabel = new QLabel( title, beam );
    QFont titleFont = titleLabel->font();
    titleFont.setBold( true );
    titleFont.setPixelSize( 15 );
    titleLabel->setFont( titleFont );
    layout->addWidget( titleLabel );

    auto* descriptionLabel = new QLabel( description, beam );
    descriptionLabel->setWordWrap( true );
    layout->addWidget( descriptionLabel );
    layout->addStretch();
    return beam;
}

} // namespace

BorderBeamShowcaseWidget::BorderBeamShowcaseWidget( QWidget* parent )
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

    auto* title = new QLabel( tr( "ExBorderBeam" ), content );
    QFont titleFont = title->font();
    titleFont.setPointSize( 16 );
    titleFont.setBold( true );
    title->setFont( titleFont );
    mainLayout->addWidget( title );

    auto* description = new QLabel(
        tr( "沿圆角边框真实周长匀速移动的渐变光束。控件继承 QFrame，可直接通过 Qt 布局承载任意内容。" ),
        content );
    description->setWordWrap( true );
    mainLayout->addWidget( description );

    auto* examplesCard = makeCard( content );
    auto* examplesLayout = new QVBoxLayout( examplesCard );
    examplesLayout->setContentsMargins( 16, 16, 16, 16 );
    examplesLayout->setSpacing( 12 );
    examplesLayout->addWidget( makeSectionTitle( tr( "常用组合" ), examplesCard ) );

    auto* samplesLayout = new QHBoxLayout;
    samplesLayout->setSpacing( 16 );

    auto* adaptiveBeam = makeBeamSample( examplesCard,
                                         tr( "主题自适应" ),
                                         tr( "颜色留空时自动使用当前调色板强调色。" ) );
    adaptiveBeam->setBeamLength( 72.0 );
    adaptiveBeam->setAnimationDuration( 5200 );
    samplesLayout->addWidget( adaptiveBeam, 1 );

    auto* doubleBeam = makeBeamSample( examplesCard,
                                       tr( "双色双光束" ),
                                       tr( "多个光束由一个控件统一绘制并沿路径均匀分布。" ) );
    doubleBeam->setStartColor( QColor( QStringLiteral( "#FFAA40" ) ) );
    doubleBeam->setEndColor( QColor( QStringLiteral( "#9C40FF" ) ) );
    doubleBeam->setBeamLength( 92.0 );
    doubleBeam->setBeamWidth( 2.5 );
    doubleBeam->setBeamCount( 2 );
    doubleBeam->setAnimationDuration( 7000 );
    samplesLayout->addWidget( doubleBeam, 1 );

    auto* reverseBeam = makeBeamSample( examplesCard,
                                        tr( "反向运动" ),
                                        tr( "切换方向不会改变当前光束位置。" ) );
    reverseBeam->setStartColor( QColor( QStringLiteral( "#22D3EE" ) ) );
    reverseBeam->setEndColor( QColor( QStringLiteral( "#3B82F6" ) ) );
    reverseBeam->setDirection( ExBorderBeam::CounterClockwise );
    reverseBeam->setBeamLength( 54.0 );
    reverseBeam->setAnimationDuration( 3600 );
    samplesLayout->addWidget( reverseBeam, 1 );

    examplesLayout->addLayout( samplesLayout );

    auto* beamButton = new ExBorderBeamButton( tr( "Border Beam Button" ), examplesCard );
    beamButton->setStartColor( QColor( QStringLiteral( "#22D3EE" ) ) );
    beamButton->setEndColor( QColor( QStringLiteral( "#8B5CF6" ) ) );
    beamButton->setBeamLength( 70.0 );
    beamButton->setAnimationDuration( 3600 );
    beamButton->setMinimumWidth( 220 );
    examplesLayout->addWidget( beamButton, 0, Qt::AlignHCenter );
    mainLayout->addWidget( examplesCard );

    auto* propertiesCard = makeCard( content );
    auto* propertiesLayout = new QVBoxLayout( propertiesCard );
    propertiesLayout->setContentsMargins( 16, 16, 16, 16 );
    propertiesLayout->setSpacing( 12 );
    propertiesLayout->addWidget( makeSectionTitle( tr( "实时属性" ), propertiesCard ) );

    auto* editorLayout = new QHBoxLayout;
    editorLayout->setSpacing( 24 );

    auto* preview = new ExBorderBeam( propertiesCard );
    preview->setMinimumSize( 400, 250 );
    preview->setCornerRadius( 16.0 );
    preview->setBeamLength( 90.0 );
    preview->setBeamWidth( 2.5 );
    preview->setAnimationDuration( 5000 );

    auto* previewContent = new QVBoxLayout( preview );
    previewContent->setContentsMargins( 28, 28, 28, 28 );
    previewContent->addStretch();
    auto* previewTitle = new QLabel( tr( "Qt 原生容器" ), preview );
    QFont previewTitleFont = previewTitle->font();
    previewTitleFont.setBold( true );
    previewTitleFont.setPixelSize( 20 );
    previewTitle->setFont( previewTitleFont );
    previewContent->addWidget( previewTitle, 0, Qt::AlignHCenter );
    auto* previewText = new QLabel( tr( "内容仍由普通布局和子控件组成" ), preview );
    previewText->setAlignment( Qt::AlignCenter );
    previewContent->addWidget( previewText );
    auto* actionButton = new QPushButton( tr( "示例按钮" ), preview );
    previewContent->addWidget( actionButton, 0, Qt::AlignHCenter );
    previewContent->addStretch();
    editorLayout->addWidget( preview, 1, Qt::AlignTop );

    auto* editor = new QWidget( propertiesCard );
    auto* form = new QFormLayout( editor );
    form->setContentsMargins( 0, 0, 0, 0 );
    form->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter );
    form->setFieldGrowthPolicy( QFormLayout::ExpandingFieldsGrow );
    form->setHorizontalSpacing( 12 );
    form->setVerticalSpacing( 10 );

    auto* lengthSlider = makeSlider( editor, 10, 300, qRound( preview->beamLength() ) );
    auto* widthSlider = makeSlider( editor, 1, 16, qRound( preview->beamWidth() * 2.0 ) );
    widthSlider->setProperty( "scale", 2 );
    widthSlider->setProperty( "precision", 1 );
    auto* radiusSlider = makeSlider( editor, 0, 48, qRound( preview->cornerRadius() ) );
    auto* durationSlider = makeSlider( editor, 500, 12000, preview->animationDuration() );
    durationSlider->setSingleStep( 100 );
    durationSlider->setPageStep( 500 );
    auto* progressSlider = makeSlider( editor, 0, 100, qRound( preview->initialProgress() * 100.0 ) );
    progressSlider->setProperty( "scale", 100 );
    progressSlider->setProperty( "precision", 2 );

    auto* countSpinBox = new QSpinBox( editor );
    countSpinBox->setRange( 1, 8 );
    countSpinBox->setValue( preview->beamCount() );

    auto* directionCombo = new ExComboBox( editor );
    directionCombo->addItem( tr( "顺时针" ), ExBorderBeam::Clockwise );
    directionCombo->addItem( tr( "逆时针" ), ExBorderBeam::CounterClockwise );

    auto* themeCombo = new ExComboBox( editor );
    themeCombo->addItem( tr( "跟随应用" ), ExBorderBeam::AutoTheme );
    themeCombo->addItem( tr( "Light" ), ExBorderBeam::LightTheme );
    themeCombo->addItem( tr( "Dark" ), ExBorderBeam::DarkTheme );

    auto* startColorButton = new ExColorPickerButton( editor );
    startColorButton->setSelectedColor( preview->activeTheme().startColor );
    auto* endColorButton = new ExColorPickerButton( editor );
    endColorButton->setSelectedColor( preview->activeTheme().endColor );
    auto* backgroundColorButton = new ExColorPickerButton( editor );
    backgroundColorButton->setSelectedColor( preview->activeTheme().backgroundColor );
    auto* borderColorButton = new ExColorPickerButton( editor );
    borderColorButton->setSelectedColor( preview->activeTheme().borderColor );

    auto* animationCheck = new QCheckBox( tr( "播放动画" ), editor );
    animationCheck->setChecked( preview->isAnimationEnabled() );

    form->addRow( tr( "光束长度" ), lengthSlider );
    form->addRow( tr( "光束线宽" ), widthSlider );
    form->addRow( tr( "圆角半径" ), radiusSlider );
    form->addRow( tr( "动画周期" ), durationSlider );
    form->addRow( tr( "初始位置" ), progressSlider );
    form->addRow( tr( "光束数量" ), countSpinBox );
    form->addRow( tr( "运动方向" ), directionCombo );
    form->addRow( tr( "主题模式" ), themeCombo );
    form->addRow( tr( "起始颜色" ), startColorButton );
    form->addRow( tr( "结束颜色" ), endColorButton );
    form->addRow( tr( "背景颜色" ), backgroundColorButton );
    form->addRow( tr( "边框颜色" ), borderColorButton );
    form->addRow( animationCheck );

    auto* resetButton = new QPushButton( tr( "恢复默认属性" ), editor );
    form->addRow( resetButton );
    editorLayout->addWidget( editor, 1 );
    propertiesLayout->addLayout( editorLayout );
    mainLayout->addWidget( propertiesCard );
    mainLayout->addStretch();

    connect( lengthSlider, &QSlider::valueChanged, preview, [preview]( int value )
             { preview->setBeamLength( value ); } );
    connect( widthSlider, &QSlider::valueChanged, preview, [preview]( int value )
             { preview->setBeamWidth( value / 2.0 ); } );
    connect( radiusSlider, &QSlider::valueChanged, preview, [preview]( int value )
             { preview->setCornerRadius( value ); } );
    connect( durationSlider, &QSlider::valueChanged, preview, &ExBorderBeam::setAnimationDuration );
    connect( progressSlider, &QSlider::valueChanged, preview, [preview]( int value )
             { preview->setInitialProgress( value / 100.0 ); } );
    connect( countSpinBox, QOverload<int>::of( &QSpinBox::valueChanged ), preview, &ExBorderBeam::setBeamCount );
    connect( directionCombo,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             preview,
             [preview, directionCombo]( int index )
             {
                 preview->setDirection( static_cast<ExBorderBeam::Direction>( directionCombo->itemData( index ).toInt() ) );
             } );
    connect( themeCombo,
             QOverload<int>::of( &QComboBox::currentIndexChanged ),
             preview,
             [=]( int index )
             {
                 preview->setThemeMode(
                     static_cast<ExBorderBeam::ThemeMode>( themeCombo->itemData( index ).toInt() ) );
                 const ExBorderBeam::ThemeConfig theme = preview->activeTheme();
                 if ( !preview->startColor().isValid() )
                 {
                     const QSignalBlocker blocker( startColorButton );
                     startColorButton->setSelectedColor( theme.startColor );
                 }
                 if ( !preview->endColor().isValid() )
                 {
                     const QSignalBlocker blocker( endColorButton );
                     endColorButton->setSelectedColor( theme.endColor );
                 }
                 if ( !preview->backgroundColor().isValid() )
                 {
                     const QSignalBlocker blocker( backgroundColorButton );
                     backgroundColorButton->setSelectedColor( theme.backgroundColor );
                 }
                 if ( !preview->borderColor().isValid() )
                 {
                     const QSignalBlocker blocker( borderColorButton );
                     borderColorButton->setSelectedColor( theme.borderColor );
                 }
             } );
    connect( startColorButton, &ExColorPickerButton::selectedColorChanged, preview, &ExBorderBeam::setStartColor );
    connect( endColorButton, &ExColorPickerButton::selectedColorChanged, preview, &ExBorderBeam::setEndColor );
    connect( backgroundColorButton,
             &ExColorPickerButton::selectedColorChanged,
             preview,
             &ExBorderBeam::setBackgroundColor );
    connect( borderColorButton,
             &ExColorPickerButton::selectedColorChanged,
             preview,
             &ExBorderBeam::setBorderColor );
    connect( animationCheck, &QCheckBox::toggled, preview, &ExBorderBeam::setAnimationEnabled );

    connect( resetButton, &QPushButton::clicked, preview, [=]
             {
                 lengthSlider->setValue( 60 );
                 widthSlider->setValue( 4 );
                 radiusSlider->setValue( 8 );
                 durationSlider->setValue( 6000 );
                 progressSlider->setValue( 0 );
                 countSpinBox->setValue( 1 );
                 directionCombo->setCurrentIndex( directionCombo->findData( ExBorderBeam::Clockwise ) );
                 themeCombo->setCurrentIndex( themeCombo->findData( ExBorderBeam::AutoTheme ) );
                 preview->setStartColor( QColor() );
                 preview->setEndColor( QColor() );
                 preview->setBackgroundColor( QColor() );
                 preview->setBorderColor( QColor() );
                 const ExBorderBeam::ThemeConfig theme = preview->activeTheme();
                 const QSignalBlocker startBlocker( startColorButton );
                 const QSignalBlocker endBlocker( endColorButton );
                 const QSignalBlocker backgroundBlocker( backgroundColorButton );
                 const QSignalBlocker borderBlocker( borderColorButton );
                 startColorButton->setSelectedColor( theme.startColor );
                 endColorButton->setSelectedColor( theme.endColor );
                 backgroundColorButton->setSelectedColor( theme.backgroundColor );
                 borderColorButton->setSelectedColor( theme.borderColor );
                 animationCheck->setChecked( true );
                 preview->restartAnimation();
             } );

    scrollArea->setWidget( content );
}
