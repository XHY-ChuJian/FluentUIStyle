#include "feedbackshowcasewidget.h"

#include <exexpander.h>
#include <exinfobar.h>
#include <exinfobarhost.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
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

QWidget* createHeaderWidget( const QString& iconGlyph,
                             const QString& title,
                             QWidget* trailingWidget = nullptr )
{
    auto* widget = new QWidget;
    auto* layout = new QHBoxLayout( widget );
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->setSpacing( 12 );

    if ( !iconGlyph.isEmpty() )
    {
        auto* iconLabel = new QLabel( iconGlyph, widget );
        QFont iconFont( QStringLiteral( "Segoe Fluent Icons" ) );
        iconFont.setPixelSize( 20 );
        iconLabel->setFont( iconFont );
        iconLabel->setFixedWidth( 24 );
        iconLabel->setAlignment( Qt::AlignCenter );
        layout->addWidget( iconLabel );
    }

    auto* titleLabel = new QLabel( title, widget );
    QFont font = titleLabel->font();
    font.setWeight( QFont::DemiBold );
    titleLabel->setFont( font );
    layout->addWidget( titleLabel );

    layout->addStretch();

    if ( trailingWidget )
    {
        trailingWidget->setParent( widget );
        layout->addWidget( trailingWidget );
    }

    return widget;
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

    // 1. 基础设置 (默认展开)
    auto* basicExpander = new ExExpander( content );
    basicExpander->setHeader( tr( "高级设置" ) );
    basicExpander->addContentWidget( new QCheckBox( tr( "启用自动保存" ) ) );
    basicExpander->addContentWidget( new QCheckBox( tr( "启动时恢复上次会话" ) ) );

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

    // 2. 多独立面板 (账户与云存储)
    auto* multiPanelExpander = new ExExpander( content );
    auto* accountHeaderTrailing = new QLabel( tr( "已登录: developer@fluentui.org" ) );
    accountHeaderTrailing->setStyleSheet( QStringLiteral( "color: gray;" ) );
    multiPanelExpander->setHeaderWidget(
        createHeaderWidget( QStringLiteral( "\uE77B" ), tr( "账户与云存储" ), accountHeaderTrailing ) );

    // Panel 1: 个人资料与订阅
    auto* profilePanel = new QWidget;
    auto* profileLayout = new QHBoxLayout( profilePanel );
    profileLayout->setContentsMargins( 0, 0, 0, 0 );
    profileLayout->setSpacing( 12 );
    profileLayout->addWidget( new QLabel( tr( "主账户订阅：开发者专业版 (至 2027-12)" ), profilePanel ) );
    profileLayout->addStretch();
    auto* manageAccountBtn = new QPushButton( tr( "管理订阅" ), profilePanel );
    connect( manageAccountBtn, &QPushButton::clicked, this, [this]
    {
        m_infoBarHost->showInfoBar( ExInfoBar::Informational,
                                    tr( "账户管理" ),
                                    tr( "正在跳转至云端控制台订阅中心..." ),
                                    ExInfoBarHost::TopRight );
    } );
    profileLayout->addWidget( manageAccountBtn );
    multiPanelExpander->addContentWidget( profilePanel );

    // Panel 2: 存储空间占用
    auto* storagePanel = new QWidget;
    auto* storageLayout = new QVBoxLayout( storagePanel );
    storageLayout->setContentsMargins( 0, 0, 0, 0 );
    storageLayout->setSpacing( 6 );
    storageLayout->addWidget( new QLabel( tr( "云存储空间 (已使用 15.4 GB / 100 GB)" ), storagePanel ) );
    auto* storageBar = new QProgressBar( storagePanel );
    storageBar->setRange( 0, 100 );
    storageBar->setValue( 15 );
    storageBar->setTextVisible( false );
    storageBar->setFixedHeight( 6 );
    storageLayout->addWidget( storageBar );
    auto* storageDetails = new QLabel( tr( "包含：代码仓库 4.1 GB · 文档 3.1 GB · 照片与媒体 8.2 GB" ), storagePanel );
    storageDetails->setStyleSheet( QStringLiteral( "color: gray; font-size: 12px;" ) );
    storageLayout->addWidget( storageDetails );
    multiPanelExpander->addContentWidget( storagePanel );

    // Panel 3: 两步验证安全设置
    auto* securityPanel = new QWidget;
    auto* securityLayout = new QHBoxLayout( securityPanel );
    securityLayout->setContentsMargins( 0, 0, 0, 0 );
    securityLayout->setSpacing( 12 );
    auto* twoFactorBox = new QCheckBox( tr( "已启用双重身份验证 (2FA / FIDO2 Key)" ), securityPanel );
    twoFactorBox->setChecked( true );
    securityLayout->addWidget( twoFactorBox );
    securityLayout->addStretch();
    auto* configKeyBtn = new QPushButton( tr( "配置密钥" ), securityPanel );
    connect( configKeyBtn, &QPushButton::clicked, this, [this]
    {
        m_infoBarHost->showInfoBar( ExInfoBar::Success,
                                    tr( "安全中心" ),
                                    tr( "当前安全密钥已处于就绪状态。" ),
                                    ExInfoBarHost::TopRight );
    } );
    securityLayout->addWidget( configKeyBtn );
    multiPanelExpander->addContentWidget( securityPanel );
    layout->addWidget( multiPanelExpander );

    // 3. 表单与网络配置
    auto* networkExpander = new ExExpander( content );
    auto* networkHeaderStatus = new QLabel( tr( "SOCKS5: 127.0.0.1:10808" ) );
    networkHeaderStatus->setStyleSheet( QStringLiteral( "color: #0078D4;" ) );
    networkExpander->setHeaderWidget(
        createHeaderWidget( QStringLiteral( "\uE774" ), tr( "网络代理与连接" ), networkHeaderStatus ) );

    auto* networkForm = new QWidget;
    auto* networkLayout = new QVBoxLayout( networkForm );
    networkLayout->setContentsMargins( 0, 0, 0, 0 );
    networkLayout->setSpacing( 10 );

    auto* proxyTypeRow = new QHBoxLayout;
    proxyTypeRow->setSpacing( 12 );
    proxyTypeRow->addWidget( new QLabel( tr( "代理协议:" ), networkForm ) );
    auto* proxyCombo = new QComboBox( networkForm );
    proxyCombo->addItems( { QStringLiteral( "SOCKS5" ), QStringLiteral( "HTTP" ), QStringLiteral( "HTTPS" ), tr( "Direct (直连)" ) } );
    proxyTypeRow->addWidget( proxyCombo );
    proxyTypeRow->addStretch();
    networkLayout->addLayout( proxyTypeRow );

    auto* hostRow = new QHBoxLayout;
    hostRow->setSpacing( 8 );
    hostRow->addWidget( new QLabel( tr( "服务器与端口:" ), networkForm ) );
    auto* hostEdit = new QLineEdit( QStringLiteral( "127.0.0.1" ), networkForm );
    hostEdit->setPlaceholderText( tr( "主机名或 IP" ) );
    hostRow->addWidget( hostEdit, 2 );
    hostRow->addWidget( new QLabel( QStringLiteral( ":" ), networkForm ) );
    auto* portSpin = new QSpinBox( networkForm );
    portSpin->setRange( 1, 65535 );
    portSpin->setValue( 10808 );
    hostRow->addWidget( portSpin, 1 );
    networkLayout->addLayout( hostRow );

    auto* bypassBox = new QCheckBox( tr( "对本地网络及环回地址 (localhost / 127.0.0.1) 绕过代理" ), networkForm );
    bypassBox->setChecked( true );
    networkLayout->addWidget( bypassBox );

    auto* networkActions = new QHBoxLayout;
    networkActions->setSpacing( 8 );
    auto* testProxyBtn = new QPushButton( tr( "测试代理连接" ), networkForm );
    connect( testProxyBtn, &QPushButton::clicked, this, [this]
    {
        m_infoBarHost->showInfoBar( ExInfoBar::Success,
                                    tr( "网络测试" ),
                                    tr( "代理连接成功，往返延迟 14 ms。" ),
                                    ExInfoBarHost::TopRight );
    } );
    networkActions->addWidget( testProxyBtn );
    auto* saveProxyBtn = new QPushButton( tr( "保存配置" ), networkForm );
    connect( saveProxyBtn, &QPushButton::clicked, this, [this]
    {
        m_infoBarHost->showInfoBar( ExInfoBar::Success,
                                    tr( "配置已保存" ),
                                    tr( "代理网络配置已生效。" ),
                                    ExInfoBarHost::TopRight );
    } );
    networkActions->addWidget( saveProxyBtn );
    networkActions->addStretch();
    networkLayout->addLayout( networkActions );

    networkExpander->addContentWidget( networkForm );
    layout->addWidget( networkExpander );

    // 4. 外观与个性化配置
    auto* appearanceExpander = new ExExpander( content );
    appearanceExpander->setHeaderWidget(
        createHeaderWidget( QStringLiteral( "\uE790" ), tr( "外观与个性化" ), new QLabel( tr( "跟随系统 · 云母效果" ) ) ) );

    auto* appearanceWidget = new QWidget;
    auto* appearanceLayout = new QVBoxLayout( appearanceWidget );
    appearanceLayout->setContentsMargins( 0, 0, 0, 0 );
    appearanceLayout->setSpacing( 10 );

    appearanceLayout->addWidget( new QLabel( tr( "选择应用主题模式：" ), appearanceWidget ) );
    auto* themeRow = new QHBoxLayout;
    themeRow->setSpacing( 16 );
    auto* lightRadio = new QRadioButton( tr( "浅色模式" ), appearanceWidget );
    auto* darkRadio = new QRadioButton( tr( "深色模式" ), appearanceWidget );
    auto* systemRadio = new QRadioButton( tr( "跟随系统" ), appearanceWidget );
    systemRadio->setChecked( true );
    themeRow->addWidget( lightRadio );
    themeRow->addWidget( darkRadio );
    themeRow->addWidget( systemRadio );
    themeRow->addStretch();
    appearanceLayout->addLayout( themeRow );

    auto* micaBox = new QCheckBox( tr( "启用窗口云母 / 亚克力透明材质 (Mica Effect)" ), appearanceWidget );
    micaBox->setChecked( true );
    appearanceLayout->addWidget( micaBox );

    appearanceLayout->addWidget( new QLabel( tr( "选择强调色预设：" ), appearanceWidget ) );
    auto* colorRow = new QHBoxLayout;
    colorRow->setSpacing( 8 );
    const QStringList colorNames = {
        tr( "天蓝" ), tr( "薄荷绿" ), tr( "紫罗兰" ), tr( "日落橙" ), tr( "石墨灰" )
    };
    for ( const QString& name : colorNames )
    {
        auto* colorBtn = new QPushButton( name, appearanceWidget );
        connect( colorBtn, &QPushButton::clicked, this, [this, name]
        {
            m_infoBarHost->showInfoBar( ExInfoBar::Informational,
                                        tr( "强调色" ),
                                        tr( "已应用强调色主题：%1" ).arg( name ),
                                        ExInfoBarHost::TopRight );
        } );
        colorRow->addWidget( colorBtn );
    }
    colorRow->addStretch();
    appearanceLayout->addLayout( colorRow );

    appearanceExpander->addContentWidget( appearanceWidget );
    layout->addWidget( appearanceExpander );

    // 5. 自定义 Header (同步状态与操作)
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

    // 6. 过渡动画与展开速度控制
    auto* animationExpander = new ExExpander( content );
    auto* animHeaderStatus = new QLabel( tr( "已启用 · 167 ms" ) );
    animHeaderStatus->setStyleSheet( QStringLiteral( "color: gray;" ) );
    animationExpander->setHeaderWidget(
        createHeaderWidget( QStringLiteral( "\uE945" ), tr( "展开动画与过渡效果" ), animHeaderStatus ) );

    auto* animControlWidget = new QWidget;
    auto* animControlLayout = new QVBoxLayout( animControlWidget );
    animControlLayout->setContentsMargins( 0, 0, 0, 0 );
    animControlLayout->setSpacing( 10 );

    auto* animToggleBox = new QCheckBox( tr( "启用展开 / 收起平滑过渡动画" ), animControlWidget );
    animToggleBox->setChecked( animationExpander->isAnimationEnabled() );
    animControlLayout->addWidget( animToggleBox );

    auto* speedRow = new QHBoxLayout;
    speedRow->setSpacing( 10 );
    speedRow->addWidget( new QLabel( tr( "动画时长:" ), animControlWidget ) );
    auto* speedSlider = new QSlider( Qt::Horizontal, animControlWidget );
    speedSlider->setRange( 50, 800 );
    speedSlider->setValue( animationExpander->animationDuration() );
    speedRow->addWidget( speedSlider, 1 );
    auto* speedValLabel = new QLabel( QStringLiteral( "167 ms" ), animControlWidget );
    speedValLabel->setFixedWidth( 55 );
    speedRow->addWidget( speedValLabel );
    animControlLayout->addLayout( speedRow );

    connect( animToggleBox, &QCheckBox::toggled, this, [animationExpander, animHeaderStatus, speedSlider]( bool checked )
    {
        animationExpander->setAnimationEnabled( checked );
        animHeaderStatus->setText( checked ? tr( "已启用 · %1 ms" ).arg( speedSlider->value() )
                                           : tr( "已禁用动画" ) );
    } );

    connect( speedSlider, &QSlider::valueChanged, this, [animationExpander, speedValLabel, animHeaderStatus, animToggleBox]( int val )
    {
        animationExpander->setAnimationDuration( val );
        speedValLabel->setText( QStringLiteral( "%1 ms" ).arg( val ) );
        if ( animToggleBox->isChecked() )
        {
            animHeaderStatus->setText( tr( "已启用 · %1 ms" ).arg( val ) );
        }
    } );

    auto* animTip = new QLabel(
        tr( "ExExpander 内置 WinUI 3 贝塞尔曲线 (0.1, 0.9, 0.2, 1.0) 平滑插值，默认展开 333 ms，收起 167 ms。" ),
        animControlWidget );
    animTip->setStyleSheet( QStringLiteral( "color: gray; font-size: 12px;" ) );
    animTip->setWordWrap( true );
    animControlLayout->addWidget( animTip );

    animationExpander->addContentWidget( animControlWidget );
    layout->addWidget( animationExpander );

    // 7. 诊断日志与生命周期信号监听
    auto* diagnosticsExpander = new ExExpander( content );
    auto* diagStatusBadge = new QLabel( tr( "状态: 已折叠" ) );
    diagStatusBadge->setStyleSheet( QStringLiteral( "color: #0078D4; font-weight: bold;" ) );
    diagnosticsExpander->setHeaderWidget(
        createHeaderWidget( QStringLiteral( "\uE9F9" ), tr( "诊断日志与事件监听" ), diagStatusBadge ) );

    auto* diagWidget = new QWidget;
    auto* diagLayout = new QVBoxLayout( diagWidget );
    diagLayout->setContentsMargins( 0, 0, 0, 0 );
    diagLayout->setSpacing( 8 );

    auto* logBox = new QPlainTextEdit( diagWidget );
    logBox->setReadOnly( true );
    logBox->setFixedHeight( 90 );
    logBox->appendPlainText( QStringLiteral( "[%1] 监听器已就绪" )
                                 .arg( QDateTime::currentDateTime().toString( QStringLiteral( "hh:mm:ss.zzz" ) ) ) );
    diagLayout->addWidget( logBox );

    auto* diagActions = new QHBoxLayout;
    diagActions->setSpacing( 8 );
    auto* addLogBtn = new QPushButton( tr( "添加测试日志" ), diagWidget );
    connect( addLogBtn, &QPushButton::clicked, this, [logBox]
    {
        logBox->appendPlainText( QStringLiteral( "[%1] 用户手动记录一条诊断事件" )
                                     .arg( QDateTime::currentDateTime().toString( QStringLiteral( "hh:mm:ss.zzz" ) ) ) );
    } );
    diagActions->addWidget( addLogBtn );
    auto* clearLogBtn = new QPushButton( tr( "清空日志" ), diagWidget );
    connect( clearLogBtn, &QPushButton::clicked, logBox, &QPlainTextEdit::clear );
    diagActions->addWidget( clearLogBtn );
    diagActions->addStretch();
    diagLayout->addLayout( diagActions );

    connect( diagnosticsExpander, &ExExpander::expanding, this, [diagStatusBadge, logBox]
    {
        diagStatusBadge->setText( tr( "状态: 展开中..." ) );
        logBox->appendPlainText( QStringLiteral( "[%1] [Signal] expanding 触发展开事件" )
                                     .arg( QDateTime::currentDateTime().toString( QStringLiteral( "hh:mm:ss.zzz" ) ) ) );
    } );

    connect( diagnosticsExpander, &ExExpander::collapsed, this, [diagStatusBadge, logBox]
    {
        diagStatusBadge->setText( tr( "状态: 已折叠" ) );
        logBox->appendPlainText( QStringLiteral( "[%1] [Signal] collapsed 内容已完全收起" )
                                     .arg( QDateTime::currentDateTime().toString( QStringLiteral( "hh:mm:ss.zzz" ) ) ) );
    } );

    connect( diagnosticsExpander, &ExExpander::expansionFinished, this, [diagStatusBadge, logBox]( bool expanded )
    {
        diagStatusBadge->setText( expanded ? tr( "状态: 已展开" ) : tr( "状态: 已折叠" ) );
        logBox->appendPlainText( QStringLiteral( "[%1] [Signal] expansionFinished (expanded = %2)" )
                                     .arg( QDateTime::currentDateTime().toString( QStringLiteral( "hh:mm:ss.zzz" ) ) )
                                     .arg( expanded ? QStringLiteral( "true" ) : QStringLiteral( "false" ) ) );
    } );

    diagnosticsExpander->addContentWidget( diagWidget );
    layout->addWidget( diagnosticsExpander );

    // 8. 向上展开 (放置在靠近底部位置)
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
