#include "audiomaticcolordialog.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

struct PresetColorInfo
{
    const char* name;
    const char* hex;
};

const PresetColorInfo kPresetColors[] = {
    { "微软蓝", "#0078D4" },
    { "青空翠", "#00B7C3" },
    { "薄荷绿", "#107C41" },
    { "极光紫", "#8764B8" },
    { "火焰红", "#E81123" },
    { "活力橙", "#F7630C" },
    { "绚丽粉", "#E3008C" },
    { "暖金黄", "#FFB900" },
    { "草地绿", "#498205" },
    { "冰湖蓝", "#0099BC" },
    { "蓝灰靛", "#4A5459" },
    { "勃艮第", "#A4262C" }
};

constexpr int kPresetCount = sizeof( kPresetColors ) / sizeof( kPresetColors[ 0 ] );

QString buttonStyleForColor( const QColor& color, bool selected )
{
    const QString hex = color.name( QColor::HexRgb );
    if ( selected )
    {
        return QStringLiteral(
            "QToolButton {"
            "  background-color: %1;"
            "  border: 3px solid #FFFFFF;"
            "  border-radius: 8px;"
            "  outline: 2px solid #0078D4;"
            "}"
            "QToolButton:hover {"
            "  border: 3px solid #E0E0E0;"
            "}" ).arg( hex );
    }
    return QStringLiteral(
        "QToolButton {"
        "  background-color: %1;"
        "  border: 1px solid rgba(255, 255, 255, 0.2);"
        "  border-radius: 8px;"
        "}"
        "QToolButton:hover {"
        "  border: 2px solid #FFFFFF;"
        "}" ).arg( hex );
}

}  // namespace

class AudiomaticColorDialogPrivate
{
public:
    AudiomaticColorDialog* q{ nullptr };
    QColor initialColor;
    QColor currentColor;

    QPushButton* btnSystemTheme{ nullptr };
    QList<QToolButton*> presetButtons;
    QToolButton* btnCustomColor{ nullptr };
    QLabel* lblCurrentColorName{ nullptr };

    void updateSelectionUi()
    {
        const bool isSystem = !currentColor.isValid();

        if ( btnSystemTheme )
        {
            btnSystemTheme->setProperty( "accent", isSystem );
            btnSystemTheme->style()->unpolish( btnSystemTheme );
            btnSystemTheme->style()->polish( btnSystemTheme );
        }

        bool matchedPreset = false;
        for ( int i = 0; i < presetButtons.size(); ++i )
        {
            auto* btn = presetButtons[ i ];
            const QColor presetCol( QString::fromLatin1( kPresetColors[ i ].hex ) );
            const bool sel = ( !isSystem && currentColor == presetCol );
            if ( sel )
            {
                matchedPreset = true;
            }
            btn->setStyleSheet( buttonStyleForColor( presetCol, sel ) );
            btn->setText( sel ? QStringLiteral( "✓" ) : QString() );
        }

        if ( btnCustomColor )
        {
            if ( !isSystem && !matchedPreset )
            {
                btnCustomColor->setStyleSheet( buttonStyleForColor( currentColor, true ) );
                btnCustomColor->setText( QStringLiteral( "✓" ) );
            }
            else
            {
                btnCustomColor->setStyleSheet( QStringLiteral(
                    "QToolButton {"
                    "  background-color: transparent;"
                    "  border: 2px dashed rgba(255, 255, 255, 0.4);"
                    "  border-radius: 8px;"
                    "  color: #FFFFFF;"
                    "  font-size: 16px;"
                    "}"
                    "QToolButton:hover {"
                    "  border-color: #FFFFFF;"
                    "  background-color: rgba(255, 255, 255, 0.08);"
                    "}" ) );
                btnCustomColor->setText( QStringLiteral( "+" ) );
            }
        }

        if ( lblCurrentColorName )
        {
            if ( isSystem )
            {
                lblCurrentColorName->setText( QObject::tr( "当前选择: 跟随系统主题" ) );
            }
            else
            {
                QString colName;
                for ( int i = 0; i < kPresetCount; ++i )
                {
                    if ( currentColor == QColor( QString::fromLatin1( kPresetColors[ i ].hex ) ) )
                    {
                        colName = QObject::tr( kPresetColors[ i ].name );
                        break;
                    }
                }
                if ( colName.isEmpty() )
                {
                    colName = QObject::tr( "自定义颜色" );
                }
                lblCurrentColorName->setText( QObject::tr( "当前选择: %1 (%2)" ).arg( colName, currentColor.name( QColor::HexRgb ).toUpper() ) );
            }
        }
    }

    void selectColor( const QColor& col )
    {
        currentColor = col;
        updateSelectionUi();
        emit q->previewColorChanged( currentColor );
    }
};

AudiomaticColorDialog::AudiomaticColorDialog( const QColor& initialColor, QWidget* parent )
    : QDialog( parent )
    , d( new AudiomaticColorDialogPrivate )
{
    d->q            = this;
    d->initialColor = initialColor;
    d->currentColor = initialColor;

    setWindowTitle( tr( "波形颜色设置" ) );
    setWindowFlag( Qt::WindowContextHelpButtonHint, false );
    setMinimumWidth( 360 );

    auto* rootLayout = new QVBoxLayout( this );
    rootLayout->setContentsMargins( 20, 20, 20, 20 );
    rootLayout->setSpacing( 16 );

    // 1. 标题与说明
    auto* lblTitle = new QLabel( tr( "自定义波形颜色 (Accent)" ), this );
    QFont titleFont = lblTitle->font();
    titleFont.setPointSize( 13 );
    titleFont.setBold( true );
    lblTitle->setFont( titleFont );
    rootLayout->addWidget( lblTitle );

    d->lblCurrentColorName = new QLabel( this );
    d->lblCurrentColorName->setStyleSheet( QStringLiteral( "color: #A0A0A0; font-size: 12px;" ) );
    rootLayout->addWidget( d->lblCurrentColorName );

    // 2. 跟随系统按钮
    d->btnSystemTheme = new QPushButton( tr( "跟随系统主题 (默认)" ), this );
    d->btnSystemTheme->setCursor( Qt::PointingHandCursor );
    d->btnSystemTheme->setMinimumHeight( 32 );
    connect( d->btnSystemTheme, &QPushButton::clicked, this, [ this ]() {
        d->selectColor( QColor() );
    } );
    rootLayout->addWidget( d->btnSystemTheme );

    // 3. 预设颜色网格（4列 × 3行）
    auto* gridWidget = new QWidget( this );
    auto* grid = new QGridLayout( gridWidget );
    grid->setContentsMargins( 0, 0, 0, 0 );
    grid->setSpacing( 10 );

    for ( int i = 0; i < kPresetCount; ++i )
    {
        const QColor col( QString::fromLatin1( kPresetColors[ i ].hex ) );
        auto* btn = new QToolButton( gridWidget );
        btn->setFixedSize( 42, 42 );
        btn->setCursor( Qt::PointingHandCursor );
        btn->setToolTip( tr( kPresetColors[ i ].name ) );
        btn->setFont( QFont( QStringLiteral( "Segoe UI" ), 12, QFont::Bold ) );

        connect( btn, &QToolButton::clicked, this, [ this, col ]() {
            d->selectColor( col );
        } );

        grid->addWidget( btn, i / 4, i % 4 );
        d->presetButtons.append( btn );
    }
    rootLayout->addWidget( gridWidget );

    // 4. 自定义颜色行
    auto* customLayout = new QHBoxLayout();
    customLayout->setSpacing( 10 );

    d->btnCustomColor = new QToolButton( this );
    d->btnCustomColor->setFixedSize( 42, 42 );
    d->btnCustomColor->setCursor( Qt::PointingHandCursor );
    d->btnCustomColor->setToolTip( tr( "选择自定义颜色..." ) );
    d->btnCustomColor->setFont( QFont( QStringLiteral( "Segoe UI" ), 12, QFont::Bold ) );

    connect( d->btnCustomColor, &QToolButton::clicked, this, [ this ]() {
        const QColor cur = d->currentColor.isValid() ? d->currentColor : QColor( QStringLiteral( "#0078D4" ) );
        const QColor chosen = QColorDialog::getColor( cur, this, tr( "选取波形自定义颜色" ) );
        if ( chosen.isValid() )
        {
            d->selectColor( chosen );
        }
    } );

    auto* btnPickMore = new QPushButton( tr( "自定义颜色..." ), this );
    btnPickMore->setCursor( Qt::PointingHandCursor );
    btnPickMore->setMinimumHeight( 32 );
    connect( btnPickMore, &QPushButton::clicked, d->btnCustomColor, &QToolButton::click );

    customLayout->addWidget( d->btnCustomColor );
    customLayout->addWidget( btnPickMore, 1 );
    rootLayout->addLayout( customLayout );

    rootLayout->addSpacing( 8 );

    // 5. 底部按钮（确定 / 取消）
    auto* footerLayout = new QHBoxLayout();
    footerLayout->setSpacing( 10 );
    footerLayout->addStretch( 1 );

    auto* btnAccept = new QPushButton( tr( "确定" ), this );
    btnAccept->setProperty( "accent", true );
    btnAccept->setDefault( true );
    btnAccept->setMinimumWidth( 75 );
    btnAccept->setMinimumHeight( 32 );
    connect( btnAccept, &QPushButton::clicked, this, &QDialog::accept );

    auto* btnCancel = new QPushButton( tr( "取消" ), this );
    btnCancel->setMinimumWidth( 75 );
    btnCancel->setMinimumHeight( 32 );
    connect( btnCancel, &QPushButton::clicked, this, &QDialog::reject );

    footerLayout->addWidget( btnAccept );
    footerLayout->addWidget( btnCancel );
    rootLayout->addLayout( footerLayout );

    d->updateSelectionUi();
}

AudiomaticColorDialog::~AudiomaticColorDialog()
{
    delete d;
}

QColor AudiomaticColorDialog::selectedColor() const
{
    return d->currentColor;
}

void AudiomaticColorDialog::setSelectedColor( const QColor& color )
{
    d->selectColor( color );
}

void AudiomaticColorDialog::done( int result )
{
    if ( result == QDialog::Accepted )
    {
        emit colorSelected( d->currentColor );
    }
    else
    {
        // 用户取消或按 Esc，自动回滚恢复为初始颜色
        emit previewColorChanged( d->initialColor );
    }
    QDialog::done( result );
}
