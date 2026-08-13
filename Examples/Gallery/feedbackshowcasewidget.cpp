#include "feedbackshowcasewidget.h"

#include <exexpander.h>
#include <exinfobar.h>
#include <exinfobarhost.h>

#include <QCheckBox>
#include <QClipboard>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>

namespace
{
QLabel* pageTitle( const QString& text, QWidget* parent )
{
    auto* label = new QLabel( text, parent );
    QFont font = label->font();
    font.setPointSize( 16 );
    font.setBold( true );
    label->setFont( font );
    return label;
}

QWidget* textContent( const QString& text, QWidget* parent = nullptr )
{
    auto* content = new QWidget( parent );
    auto* layout = new QVBoxLayout( content );
    layout->setContentsMargins( 0, 0, 0, 0 );
    auto* label = new QLabel( text, content );
    label->setWordWrap( true );
    layout->addWidget( label );
    return content;
}
} // namespace

FeedbackShowcaseWidget::FeedbackShowcaseWidget( QWidget* parent )
    : QFrame( parent )
{
    m_infoBarHost = ExInfoBarHost::defaultHost();
    if ( !m_infoBarHost )
    {
        // 允许该页面脱离 Gallery 主窗口单独使用。
        m_infoBarHost = new ExInfoBarHost( window(), this );
    }

    setFrameShape( QFrame::StyledPanel );

    auto* root = new QVBoxLayout( this );
    root->setContentsMargins( 0, 0, 0, 0 );

    auto* scroll = new QScrollArea( this );
    scroll->setWidgetResizable( true );
    scroll->setFrameShape( QFrame::NoFrame );
    scroll->viewport()->setAutoFillBackground( false );
    root->addWidget( scroll );

    auto* content = new QWidget( scroll );
    auto* layout = new QVBoxLayout( content );
    layout->setContentsMargins( 20, 20, 20, 20 );
    layout->setSpacing( 12 );

    layout->addWidget( pageTitle( tr( "InfoBar 与 Expander" ), content ) );
    auto* introduction = new QLabel(
        tr( "InfoBar 用于页面内非阻塞通知；Expander 用于按需显示相关的次要内容。" ), content );
    introduction->setWordWrap( true );
    layout->addWidget( introduction );

    auto* infoTitle = new QLabel( tr( "ExInfoBar" ), content );
    QFont sectionFont = infoTitle->font();
    sectionFont.setBold( true );
    infoTitle->setFont( sectionFont );
    layout->addWidget( infoTitle );

    auto makeInfoBar = [content]( ExInfoBar::Severity severity,
                                  const QString& title,
                                  const QString& message )
    {
        auto* bar = new ExInfoBar( content );
        bar->setSeverity( severity );
        bar->setTitle( title );
        bar->setMessage( message );
        bar->setOpen( true );
        return bar;
    };

    auto* information = makeInfoBar( ExInfoBar::Informational,
                                     tr( "信息" ),
                                     tr( "新版本已经可以下载。" ) );
    information->setActionButtonText( tr( "查看更新" ) );
    connect( information, &ExInfoBar::actionTriggered, information, [this, information]()
    {
        information->setMessage( tr( "当前已经是最新版本。" ) );
        information->setActionButtonText( QString() );
    } );

    auto* success = makeInfoBar( ExInfoBar::Success, tr( "成功" ), tr( "所有更改均已保存。" ) );
    auto* warning = makeInfoBar( ExInfoBar::Warning, tr( "警告" ), tr( "网络连接不稳定，部分内容可能延迟。" ) );
    auto* error = makeInfoBar( ExInfoBar::Error, tr( "错误" ), tr( "无法连接到服务，请稍后重试。" ) );

    layout->addWidget( information );
    layout->addWidget( success );
    layout->addWidget( warning );
    layout->addWidget( error );

    auto* reopenButton = new QPushButton( tr( "重新显示全部通知" ), content );
    connect( reopenButton, &QPushButton::clicked, this, [information, success, warning, error]()
    {
        information->setOpen( true );
        success->setOpen( true );
        warning->setOpen( true );
        error->setOpen( true );
    } );
    layout->addWidget( reopenButton, 0, Qt::AlignLeft );

    auto* popupTitle = new QLabel( tr( "窗口级弹出" ), content );
    popupTitle->setFont( sectionFont );
    layout->addSpacing( 8 );
    layout->addWidget( popupTitle );

    auto* popupDescription = new QLabel(
        tr( "弹出通知相对主窗口定位；顶部的新通知向下追加，底部的新通知向上追加，空间不足时自动排队。" ),
        content );
    popupDescription->setWordWrap( true );
    layout->addWidget( popupDescription );

    auto* positions = new QWidget( content );
    auto* positionLayout = new QGridLayout( positions );
    positionLayout->setContentsMargins( 0, 0, 0, 0 );
    positionLayout->setHorizontalSpacing( 8 );
    positionLayout->setVerticalSpacing( 8 );

    auto showPopup = [this]( ExInfoBarHost::Position position )
    {
        const int serial = ++m_popupSerial;
        const auto severity = static_cast<ExInfoBar::Severity>( ( serial - 1 ) % 4 );
        m_infoBarHost->showInfoBar(
            severity,
            tr( "通知%1" ).arg( serial ),
            tr( "该通知会在 4.5 秒后关闭；鼠标悬停时暂停计时。" ),
            position );
    };

    struct PositionButton
    {
        const char* text;
        ExInfoBarHost::Position position;
        int row;
        int column;
    };
    const PositionButton positionButtons[] = {
        { QT_TR_NOOP( "左上" ), ExInfoBarHost::TopLeft, 0, 0 },
        { QT_TR_NOOP( "顶部" ), ExInfoBarHost::Top, 0, 1 },
        { QT_TR_NOOP( "右上" ), ExInfoBarHost::TopRight, 0, 2 },
        { QT_TR_NOOP( "左下" ), ExInfoBarHost::BottomLeft, 1, 0 },
        { QT_TR_NOOP( "底部" ), ExInfoBarHost::Bottom, 1, 1 },
        { QT_TR_NOOP( "右下" ), ExInfoBarHost::BottomRight, 1, 2 }
    };
    for ( const PositionButton& item : positionButtons )
    {
        auto* button = new QPushButton( tr( item.text ), positions );
        connect( button, &QPushButton::clicked, this, [showPopup, item]
        {
            showPopup( item.position );
        } );
        positionLayout->addWidget( button, item.row, item.column );
    }
    layout->addWidget( positions );

    auto* popupActions = new QWidget( content );
    auto* popupActionLayout = new QHBoxLayout( popupActions );
    popupActionLayout->setContentsMargins( 0, 0, 0, 0 );
    popupActionLayout->setSpacing( 8 );
    auto* multipleButton = new QPushButton( tr( "在右上角连续弹出 10 条" ), popupActions );
    connect( multipleButton, &QPushButton::clicked, this, [showPopup]
    {
        for ( int index = 0; index < 10; ++index )
        {
            showPopup( ExInfoBarHost::TopRight );
        }
    } );
    popupActionLayout->addWidget( multipleButton );
    auto* dismissPopupButton = new QPushButton( tr( "关闭全部弹出通知" ), popupActions );
    connect( dismissPopupButton, &QPushButton::clicked,
             m_infoBarHost, qOverload<>( &ExInfoBarHost::dismissAll ) );
    popupActionLayout->addWidget( dismissPopupButton );
    popupActionLayout->addStretch();
    layout->addWidget( popupActions );

    auto* expanderTitle = new QLabel( tr( "ExExpander" ), content );
    expanderTitle->setFont( sectionFont );
    layout->addSpacing( 8 );
    layout->addWidget( expanderTitle );

    auto* basicExpander = new ExExpander( content );
    basicExpander->setHeader( tr( "高级设置" ) );
    basicExpander->addContentWidget( new QCheckBox( tr( "启用自动保存" ) ));
    basicExpander->addContentWidget( new QCheckBox( tr( "启动时恢复上次会话" ) ));

    auto* audioSettings = new QWidget;
    auto* audioSettingsLayout = new QVBoxLayout( audioSettings );
    audioSettingsLayout->setContentsMargins( 0, 0, 0, 0 );
    audioSettingsLayout->setSpacing( 8 );
    auto* volume = new QSlider( Qt::Horizontal, audioSettings );
    volume->setValue( 60 );
    audioSettingsLayout->addWidget( new QLabel( tr( "通知音量" ), audioSettings ) );
    audioSettingsLayout->addWidget( volume );
    basicExpander->addContentWidget( audioSettings );
    basicExpander->setExpanded( true );
    layout->addWidget( basicExpander );

    auto* customHeaderExpander = new ExExpander( content );
    auto* customHeader = new QWidget;
    auto* headerLayout = new QHBoxLayout( customHeader );
    headerLayout->setContentsMargins( 0, 0, 0, 0 );
    headerLayout->setSpacing( 12 );
    auto* headerIcon = new QLabel( QStringLiteral( "\uE713" ), customHeader );
    QFont iconFont( QStringLiteral( "Segoe Fluent Icons" ) );
    iconFont.setPixelSize( 20 );
    headerIcon->setFont( iconFont );
    headerIcon->setFixedWidth( 24 );
    headerIcon->setAlignment( Qt::AlignCenter );
    headerLayout->addWidget( headerIcon );
    auto* headerText = new QLabel( tr( "同步状态" ), customHeader );
    QFont headerFont = headerText->font();
    headerFont.setWeight( QFont::DemiBold );
    headerText->setFont( headerFont );
    headerLayout->addWidget( headerText );
    headerLayout->addStretch();
    auto* copyButton = new QPushButton( tr( "复制" ), customHeader );
    connect( copyButton, &QPushButton::clicked, this, [this]
    {
        QGuiApplication::clipboard()->setText( tr( "3 个设备" ) );
    } );
    headerLayout->addWidget( copyButton );
    customHeaderExpander->setHeaderWidget( customHeader );
    customHeaderExpander->addContentWidget(
        textContent( tr( "桌面电脑、笔记本电脑和移动设备均已在刚刚完成同步。" ) ) );
    layout->addWidget( customHeaderExpander );

    auto* upwardExpander = new ExExpander( content );
    upwardExpander->setHeader( tr( "向上展开" ) );
    upwardExpander->setExpandDirection( ExExpander::Up );
    upwardExpander->addContentWidget(
        textContent( tr( "内容显示在 Header 上方，适合靠近页面底部的布局。" ) ) );
    layout->addWidget( upwardExpander );

    layout->addStretch( 1 );
    scroll->setWidget( content );
    content->setAutoFillBackground( false );
}
