#include "systemresourceswidget.h"

#include "systemresources/systemresourceprovider.h"

#include <exmultiprogressring.h>
#include <exmultiradialgauge.h>
#include <exliquidgauge.h>
#include <exprogressring.h>
#include <exradialgauge.h>
#include <extimeline.h>

#include <QComboBox>
#include <QDateTime>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QShowEvent>
#include <QStringList>
#include <QThread>
#include <QVBoxLayout>

#include <limits>

namespace
{
QFrame* makeCard( QWidget* parent )
{
    auto* card = new QFrame( parent );
    card->setProperty( "isCard", true );
    card->setAttribute( Qt::WA_StyledBackground, true );
    card->setFrameShape( QFrame::NoFrame );
    return card;
}

QLabel* makeTitle( const QString& text, QWidget* parent, int pixelSize = 14 )
{
    auto* label = new QLabel( text, parent );
    QFont font = label->font();
    font.setBold( true );
    font.setPixelSize( pixelSize );
    label->setFont( font );
    return label;
}

void configureDetailsLabel( QLabel* label )
{
    label->setAlignment( Qt::AlignCenter );
    label->setProperty( "isSecondaryText", true );
    label->setWordWrap( true );
}

QString formatBytes( quint64 bytes, bool perSecond = false )
{
    static const QStringList units{ QStringLiteral( "B" ),
                                    QStringLiteral( "KB" ),
                                    QStringLiteral( "MB" ),
                                    QStringLiteral( "GB" ),
                                    QStringLiteral( "TB" ) };
    qreal value = static_cast<qreal>( bytes );
    int unitIndex = 0;
    while ( value >= 1024.0 && unitIndex < units.size() - 1 )
    {
        value /= 1024.0;
        ++unitIndex;
    }
    const int decimals = value >= 100.0 || unitIndex == 0 ? 0 : 1;
    return QStringLiteral( "%1 %2%3" )
        .arg( QString::number( value, 'f', decimals ), units.at( unitIndex ),
              perSecond ? QStringLiteral( "/s" ) : QString() );
}

QString formatUptime( quint64 seconds )
{
    const quint64 days = seconds / 86400;
    seconds %= 86400;
    const quint64 hours = seconds / 3600;
    const quint64 minutes = seconds % 3600 / 60;
    if ( days > 0 )
    {
        return QObject::tr( "%1 天 %2 小时" ).arg( days ).arg( hours );
    }
    return QObject::tr( "%1 小时 %2 分" ).arg( hours ).arg( minutes );
}

QColor accentColor( const QWidget* widget )
{
#if QT_VERSION >= QT_VERSION_CHECK( 6, 6, 0 )
    return widget->palette().color( QPalette::Accent );
#else
    return widget->palette().color( QPalette::Highlight );
#endif
}

QColor diskColor( int index, const QWidget* widget )
{
    const QList<QColor> colors{ accentColor( widget ),
                                QColor( 0x00, 0xB7, 0xC3 ),
                                QColor( 0x6C, 0xB5, 0x3F ),
                                QColor( 0xC2, 0x8B, 0x00 ) };
    return colors.at( index % colors.size() );
}

quint64 networkScaleForRate( quint64 bytesPerSecond )
{
    constexpr quint64 minimumScale = 32 * 1024;
    const quint64 headroom = bytesPerSecond / 4;
    const quint64 target = bytesPerSecond > std::numeric_limits<quint64>::max() - headroom
        ? std::numeric_limits<quint64>::max()
        : bytesPerSecond + headroom;

    quint64 scale = minimumScale;
    while ( scale < target && scale <= std::numeric_limits<quint64>::max() / 2 )
    {
        scale *= 2;
    }
    return scale;
}
}

SystemResourcesWidget::SystemResourcesWidget( QWidget* parent )
    : QFrame( parent )
    , m_provider( new SystemResourceProvider( this ) )
{
    setFrameShape( QFrame::StyledPanel );

    auto* rootLayout = new QVBoxLayout( this );
    rootLayout->setContentsMargins( 0, 0, 0, 0 );

    auto* scrollArea = new QScrollArea( this );
    scrollArea->setWidgetResizable( true );
    scrollArea->setFrameShape( QFrame::NoFrame );
    scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    scrollArea->setAutoFillBackground( false );
    scrollArea->viewport()->setAutoFillBackground( false );
    rootLayout->addWidget( scrollArea );

    auto* content = new QWidget( scrollArea );
    content->setAutoFillBackground( false );
    auto* mainLayout = new QVBoxLayout( content );
    mainLayout->setContentsMargins( 16, 16, 16, 16 );
    mainLayout->setSpacing( 16 );

    auto* headerLayout = new QHBoxLayout;
    headerLayout->setSpacing( 10 );
    auto* headingLayout = new QVBoxLayout;
    headingLayout->setSpacing( 4 );
    headingLayout->addWidget( makeTitle( tr( "系统资源监视器" ), content, 20 ) );
    auto* description = new QLabel(
        tr( "使用多种 ExWidgets 控件实时展示 CPU、内存、网络、磁盘、GPU 和资源告警。" ),
        content );
    description->setProperty( "isSecondaryText", true );
    description->setWordWrap( true );
    headingLayout->addWidget( description );
    headerLayout->addLayout( headingLayout, 1 );

    m_statusLabel = new QLabel( tr( "等待采样" ), content );
    m_statusLabel->setProperty( "isSecondaryText", true );
    headerLayout->addWidget( m_statusLabel );

    m_intervalCombo = new QComboBox( content );
    m_intervalCombo->addItem( tr( "1 秒" ), 1000 );
    m_intervalCombo->addItem( tr( "2 秒" ), 2000 );
    m_intervalCombo->addItem( tr( "5 秒" ), 5000 );
    m_intervalCombo->setCurrentIndex( 0 );
    headerLayout->addWidget( m_intervalCombo );

    m_pauseButton = new QPushButton( tr( "暂停" ), content );
    m_pauseButton->setCheckable( true );
    headerLayout->addWidget( m_pauseButton );
    mainLayout->addLayout( headerLayout );

    auto* cardsLayout = new QGridLayout;
    cardsLayout->setHorizontalSpacing( 16 );
    cardsLayout->setVerticalSpacing( 16 );
    cardsLayout->setColumnStretch( 0, 1 );
    cardsLayout->setColumnStretch( 1, 1 );

    auto* cpuCard = makeCard( content );
    auto* cpuLayout = new QVBoxLayout( cpuCard );
    cpuLayout->setContentsMargins( 16, 16, 16, 16 );
    cpuLayout->setSpacing( 8 );
    cpuLayout->addWidget( makeTitle( tr( "处理器" ), cpuCard ) );
    m_cpuGauge = new ExRadialGauge( cpuCard );
    m_cpuGauge->setRange( 0, 100 );
    m_cpuGauge->setInteractive( false );
    m_cpuGauge->setScaleMode( ExRadialGauge::ProgressScale );
    m_cpuGauge->setNeedleStyle( ExRadialGauge::TriangleNeedle );
    m_cpuGauge->setHubVisible( true );
    m_cpuGauge->setMajorTickCount( 6 );
    m_cpuGauge->setMinorTickCount( 4 );
    m_cpuGauge->setLabelsVisible( true );
    m_cpuGauge->setProgressGradientEnabled( true );
    m_cpuGauge->setTitle( QStringLiteral( "CPU" ) );
    m_cpuGauge->setUnit( QStringLiteral( "%" ) );
    m_cpuGauge->setValueAnimationDuration( 400 );
    m_cpuGauge->setFixedSize( 250, 250 );
    cpuLayout->addWidget( m_cpuGauge, 1, Qt::AlignHCenter );
    m_cpuDetails = new QLabel( tr( "正在获取处理器使用率…" ), cpuCard );
    configureDetailsLabel( m_cpuDetails );
    cpuLayout->addWidget( m_cpuDetails );
    cardsLayout->addWidget( cpuCard, 0, 0 );

    auto* memoryCard = makeCard( content );
    auto* memoryLayout = new QVBoxLayout( memoryCard );
    memoryLayout->setContentsMargins( 16, 16, 16, 16 );
    memoryLayout->setSpacing( 8 );
    memoryLayout->addWidget( makeTitle( tr( "内存" ), memoryCard ) );
    m_memoryGauge = new ExLiquidGauge( memoryCard );
    m_memoryGauge->setRange( 0, 100 );
    m_memoryGauge->setValue( 0 );
    m_memoryGauge->setShape( ExLiquidGauge::CircleShape );
    m_memoryGauge->setFormat( QStringLiteral( "%p%" ) );
    m_memoryGauge->setAlignment( Qt::AlignCenter );
    m_memoryGauge->setWaveAmplitude( 7.0 );
    m_memoryGauge->setWaveCount( 2 );
    m_memoryGauge->setFixedSize( 220, 220 );
    memoryLayout->addWidget( m_memoryGauge, 1, Qt::AlignHCenter );
    m_memoryDetails = new QLabel( tr( "正在获取物理内存…" ), memoryCard );
    configureDetailsLabel( m_memoryDetails );
    memoryLayout->addWidget( m_memoryDetails );
    cardsLayout->addWidget( memoryCard, 0, 1 );

    auto* networkCard = makeCard( content );
    auto* networkLayout = new QVBoxLayout( networkCard );
    networkLayout->setContentsMargins( 16, 16, 16, 16 );
    networkLayout->setSpacing( 8 );
    networkLayout->addWidget( makeTitle( tr( "网络" ), networkCard ) );
    m_networkGauge = new ExMultiRadialGauge( networkCard );
    m_networkGauge->setRange( 0.0, 32.0 );
    m_networkGauge->setMinimumAngle( -120.0 );
    m_networkGauge->setMaximumAngle( 120.0 );
    m_networkGauge->setMajorTickCount( 5 );
    m_networkGauge->setMinorTickCount( 1 );
    m_networkGauge->setProgressOverlap( false );
    m_networkGauge->setProgressWidth( 7.0 );
    m_networkGauge->setNeedleStyle( ExMultiRadialGauge::LineNeedle );
    m_networkGauge->setNeedleWidth( 3.0 );
    m_networkGauge->setValueSuffix( QStringLiteral( " KB/s" ) );
    m_networkGauge->setValueDecimals( 1 );
    m_networkGauge->setValueAnimationDuration( 400 );
    m_networkGauge->setFixedSize( 270, 270 );
    m_receiveItem = m_networkGauge->addItem( tr( "下载" ), 0.0, QColor( 0x00, 0xB7, 0xC3 ) );
    m_receiveItem->setTitleOffset( QPointF( -0.34, 0.58 ) );
    m_receiveItem->setDetailOffset( QPointF( -0.34, 0.76 ) );
    m_sendItem = m_networkGauge->addItem( tr( "上传" ), 0.0, QColor( 0x6C, 0xB5, 0x3F ) );
    m_sendItem->setTitleOffset( QPointF( 0.34, 0.58 ) );
    m_sendItem->setDetailOffset( QPointF( 0.34, 0.76 ) );
    networkLayout->addWidget( m_networkGauge, 1, Qt::AlignHCenter );
    m_networkDetails = new QLabel( tr( "正在获取网络流量…" ), networkCard );
    configureDetailsLabel( m_networkDetails );
    networkLayout->addWidget( m_networkDetails );
    cardsLayout->addWidget( networkCard, 1, 0 );

    auto* diskCard = makeCard( content );
    auto* diskLayout = new QVBoxLayout( diskCard );
    diskLayout->setContentsMargins( 16, 16, 16, 16 );
    diskLayout->setSpacing( 8 );
    diskLayout->addWidget( makeTitle( tr( "磁盘活动" ), diskCard ) );
    m_diskRing = new ExMultiProgressRing( diskCard );
    m_diskRing->setRange( 0.0, 100.0 );
    m_diskRing->setTrackVisible( true );
    m_diskRing->setRingWidth( 9.0 );
    m_diskRing->setRingSpacing( 5.0 );
    m_diskRing->setValueAnimationDuration( 400 );
    m_diskRing->setFixedSize( 270, 270 );
    diskLayout->addWidget( m_diskRing, 1, Qt::AlignHCenter );
    m_diskDetails = new QLabel( tr( "正在采样磁盘活动率…" ), diskCard );
    configureDetailsLabel( m_diskDetails );
    diskLayout->addWidget( m_diskDetails );
    cardsLayout->addWidget( diskCard, 1, 1 );

    auto* gpuCard = makeCard( content );
    auto* gpuLayout = new QVBoxLayout( gpuCard );
    gpuLayout->setContentsMargins( 16, 16, 16, 16 );
    gpuLayout->setSpacing( 8 );
    gpuLayout->addWidget( makeTitle( tr( "图形处理器" ), gpuCard ) );
    m_gpuRing = new ExProgressRing( gpuCard );
    m_gpuRing->setRange( 0, 100 );
    m_gpuRing->setValue( 0 );
    m_gpuRing->setTitle( QStringLiteral( "GPU" ) );
    m_gpuRing->setFormat( QStringLiteral( "%p%" ) );
    m_gpuRing->setAlignment( Qt::AlignCenter );
    m_gpuRing->setFixedSize( 220, 220 );
    gpuLayout->addWidget( m_gpuRing, 1, Qt::AlignHCenter );
    m_gpuDetails = new QLabel( tr( "正在采样 GPU 利用率…" ), gpuCard );
    configureDetailsLabel( m_gpuDetails );
    gpuLayout->addWidget( m_gpuDetails );
    cardsLayout->addWidget( gpuCard, 2, 0, 1, 2 );
    mainLayout->addLayout( cardsLayout );

    auto* timelineCard = makeCard( content );
    auto* timelineLayout = new QVBoxLayout( timelineCard );
    timelineLayout->setContentsMargins( 16, 16, 16, 16 );
    timelineLayout->setSpacing( 8 );
    timelineLayout->addWidget( makeTitle( tr( "资源事件" ), timelineCard ) );
    auto* timelineDescription = new QLabel(
        tr( "CPU、内存、磁盘或 GPU 达到高占用阈值时，会在这里记录告警和恢复事件。" ),
        timelineCard );
    timelineDescription->setProperty( "isSecondaryText", true );
    timelineLayout->addWidget( timelineDescription );
    m_timeline = new ExTimeline( timelineCard );
    m_timeline->setLayoutMode( ExTimeline::ContentOnRight );
    m_timeline->setReverse( true );
    m_timeline->setTimestampFormat( QStringLiteral( "HH:mm:ss" ) );
    m_timeline->setTimestampWidth( 76 );
    m_timeline->setMinimumHeight( 260 );
    timelineLayout->addWidget( m_timeline );
    mainLayout->addWidget( timelineCard );
    mainLayout->addStretch();

    scrollArea->setWidget( content );

    connect( m_provider,
             &SystemResourceProvider::snapshotReady,
             this,
             &SystemResourcesWidget::applySnapshot );
    connect( m_intervalCombo,
             qOverload<int>( &QComboBox::currentIndexChanged ),
             this,
             [this]
             {
                 m_provider->setInterval( samplingInterval() );
             } );
    connect( m_pauseButton,
             &QPushButton::toggled,
             this,
             [this]( bool paused )
             {
                 m_paused = paused;
                 m_pauseButton->setText( paused ? tr( "继续" ) : tr( "暂停" ) );
                 if ( paused )
                 {
                     m_provider->stop();
                     m_statusLabel->setText( tr( "已暂停" ) );
                 }
                 else if ( isVisible() )
                 {
                     m_provider->start( samplingInterval() );
                 }
             } );

    appendTimelineEvent( tr( "监视器已就绪" ),
                         tr( "打开此页面后开始采集系统资源。" ),
                         ExTimelineEvent::Completed );
}

void SystemResourcesWidget::showEvent( QShowEvent* event )
{
    QFrame::showEvent( event );
    if ( !m_paused )
    {
        m_provider->start( samplingInterval() );
    }
}

void SystemResourcesWidget::hideEvent( QHideEvent* event )
{
    m_provider->stop();
    QFrame::hideEvent( event );
}

void SystemResourcesWidget::applySnapshot( const SystemResourceSnapshot& snapshot )
{
    if ( !snapshot.available )
    {
        m_statusLabel->setText( tr( "当前平台暂不支持" ) );
        m_cpuDetails->setText( tr( "系统指标仅在 Windows 上可用" ) );
        m_memoryDetails->setText( tr( "系统指标仅在 Windows 上可用" ) );
        m_networkDetails->setText( tr( "系统指标仅在 Windows 上可用" ) );
        m_diskDetails->setText( tr( "系统指标仅在 Windows 上可用" ) );
        m_gpuDetails->setText( tr( "系统指标仅在 Windows 上可用" ) );
        return;
    }

    m_statusLabel->setText( tr( "更新于 %1" ).arg( snapshot.timestamp.time().toString( QStringLiteral( "HH:mm:ss" ) ) ) );

    if ( snapshot.cpuUsage >= 0.0 )
    {
        m_cpuGauge->setValue( qRound( snapshot.cpuUsage ) );
        m_cpuDetails->setText(
            tr( "%1 个逻辑处理器 · 已运行 %2" )
                .arg( qMax( 1, QThread::idealThreadCount() ) )
                .arg( formatUptime( snapshot.uptimeSeconds ) ) );
        updateAlert( QStringLiteral( "cpu" ),
                     snapshot.cpuUsage >= ( m_alertStates.value( QStringLiteral( "cpu" ) ) ? 75.0 : 85.0 ),
                     tr( "CPU 高占用" ),
                     tr( "处理器使用率为 %1%。" ).arg( snapshot.cpuUsage, 0, 'f', 0 ) );
    }

    m_memoryGauge->setValue( qRound( snapshot.memoryUsage ) );
    const quint64 usedMemory = snapshot.totalMemoryBytes - snapshot.availableMemoryBytes;
    m_memoryDetails->setText(
        tr( "%1 / %2 · 可用 %3" )
            .arg( formatBytes( usedMemory ),
                  formatBytes( snapshot.totalMemoryBytes ),
                  formatBytes( snapshot.availableMemoryBytes ) ) );
    updateAlert( QStringLiteral( "memory" ),
                 snapshot.memoryUsage >= ( m_alertStates.value( QStringLiteral( "memory" ) ) ? 85.0 : 90.0 ),
                 tr( "内存高占用" ),
                 tr( "物理内存使用率为 %1%。" ).arg( snapshot.memoryUsage, 0, 'f', 0 ) );

    if ( snapshot.network.rateAvailable )
    {
        updateNetworkGauge( snapshot.network.receivedBytesPerSecond,
                            snapshot.network.sentBytesPerSecond );
    }
    if ( snapshot.network.connected )
    {
        m_networkDetails->setText( snapshot.network.rateAvailable
            ? tr( "%1 · 下载 %2 · 上传 %3" )
                  .arg( snapshot.network.name,
                        formatBytes( snapshot.network.receivedBytesPerSecond, true ),
                        formatBytes( snapshot.network.sentBytesPerSecond, true ) )
            : tr( "%1 · 正在计算流量…" ).arg( snapshot.network.name ) );
    }
    else
    {
        m_receiveItem->setValue( 0.0 );
        m_sendItem->setValue( 0.0 );
        m_networkDetails->setText( tr( "没有检测到活动网络" ) );
    }
    updateAlert( QStringLiteral( "network" ),
                 !snapshot.network.connected,
                 tr( "网络已断开" ),
                 tr( "没有检测到已连接的网络适配器。" ) );

    synchronizeDiskItems( snapshot );

    if ( !snapshot.performanceSampleReady )
    {
        m_gpuDetails->setText( tr( "正在采样 GPU 利用率…" ) );
    }
    else if ( snapshot.gpuAvailable && snapshot.gpuUsage >= 0.0 )
    {
        m_gpuRing->setEnabled( true );
        m_gpuRing->setValue( qRound( snapshot.gpuUsage ) );
        m_gpuDetails->setText( snapshot.gpuEngineName.isEmpty()
            ? tr( "总体利用率 %1%" ).arg( snapshot.gpuUsage, 0, 'f', 1 )
            : tr( "总体利用率 %1% · 最忙引擎 %2" )
                  .arg( snapshot.gpuUsage, 0, 'f', 1 )
                  .arg( snapshot.gpuEngineName ) );
        updateAlert( QStringLiteral( "gpu" ),
                     snapshot.gpuUsage >= ( m_alertStates.value( QStringLiteral( "gpu" ) ) ? 85.0 : 90.0 ),
                     tr( "GPU 高占用" ),
                     tr( "GPU 总体利用率为 %1%。" ).arg( snapshot.gpuUsage, 0, 'f', 0 ) );
    }
    else
    {
        m_gpuRing->setValue( 0 );
        m_gpuRing->setEnabled( false );
        m_gpuDetails->setText( tr( "当前系统未提供 GPU 性能计数器" ) );
    }
}

void SystemResourcesWidget::updateNetworkGauge( quint64 receivedBytesPerSecond,
                                                quint64 sentBytesPerSecond )
{
    const quint64 peakRate = qMax( receivedBytesPerSecond, sentBytesPerSecond );
    const quint64 desiredScale = networkScaleForRate( peakRate );
    bool scaleChanged = false;

    if ( desiredScale > m_networkScaleBytesPerSecond )
    {
        m_networkScaleBytesPerSecond = desiredScale;
        m_networkLowTrafficSamples = 0;
        scaleChanged = true;
    }
    else if ( desiredScale < m_networkScaleBytesPerSecond &&
              peakRate < m_networkScaleBytesPerSecond / 4 )
    {
        if ( ++m_networkLowTrafficSamples >= 10 )
        {
            m_networkScaleBytesPerSecond = desiredScale;
            m_networkLowTrafficSamples = 0;
            scaleChanged = true;
        }
    }
    else
    {
        m_networkLowTrafficSamples = 0;
    }

    constexpr qreal kibibyte = 1024.0;
    constexpr qreal mebibyte = 1024.0 * 1024.0;
    constexpr qreal gibibyte = 1024.0 * 1024.0 * 1024.0;
    qreal divisor = kibibyte;
    QString suffix = QStringLiteral( " KB/s" );
    int decimals = 1;
    if ( m_networkScaleBytesPerSecond >= static_cast<quint64>( gibibyte ) )
    {
        divisor = gibibyte;
        suffix = QStringLiteral( " GB/s" );
        decimals = 2;
    }
    else if ( m_networkScaleBytesPerSecond >= static_cast<quint64>( mebibyte ) )
    {
        divisor = mebibyte;
        suffix = QStringLiteral( " MB/s" );
        decimals = 2;
    }

    const int animationDuration = m_networkGauge->valueAnimationDuration();
    if ( scaleChanged )
    {
        m_networkGauge->setValueAnimationDuration( 0 );
    }
    m_networkGauge->setRange( 0.0, m_networkScaleBytesPerSecond / divisor );
    m_networkGauge->setValueSuffix( suffix );
    m_networkGauge->setValueDecimals( decimals );
    m_receiveItem->setValue( receivedBytesPerSecond / divisor );
    m_sendItem->setValue( sentBytesPerSecond / divisor );
    if ( scaleChanged )
    {
        m_networkGauge->setValueAnimationDuration( animationDuration );
    }
}

void SystemResourcesWidget::synchronizeDiskItems( const SystemResourceSnapshot& snapshot )
{
    constexpr int maximumVisibleDisks = 4;
    for ( const SystemDiskSnapshot& disk : snapshot.disks )
    {
        const QString alertKey = QStringLiteral( "disk:%1" ).arg( disk.name );
        updateAlert( alertKey,
                     disk.usedPercent >= ( m_alertStates.value( alertKey ) ? 85.0 : 90.0 ),
                     tr( "%1 空间不足" ).arg( disk.name ),
                     tr( "磁盘空间使用率为 %1%，剩余 %2。" )
                         .arg( disk.usedPercent, 0, 'f', 0 )
                         .arg( formatBytes( disk.availableBytes ) ) );
    }

    if ( snapshot.performanceSampleReady )
    {
        QSet<QString> visibleDisks;
        const int visibleDiskCount = snapshot.diskActivityAvailable
            ? qMin( static_cast<int>( snapshot.diskActivities.size() ), maximumVisibleDisks )
            : 0;
        for ( int index = 0; index < visibleDiskCount; ++index )
        {
            const SystemDiskActivitySnapshot& disk = snapshot.diskActivities.at( index );
            visibleDisks.insert( disk.name );
            const QString diskNumber = disk.name.section( QLatin1Char( ' ' ), 0, 0 );
            const QString label = disk.name == QStringLiteral( "_Total" )
                ? tr( "总计" )
                : tr( "磁盘 %1" ).arg( diskNumber );
            ExMultiProgressRingItem* item = m_diskItems.value( disk.name );
            if ( !item )
            {
                item = m_diskRing->addItem( label,
                                            disk.usagePercent,
                                            diskColor( index, m_diskRing ) );
                m_diskItems.insert( disk.name, item );
            }
            else
            {
                item->setLabel( label );
                item->setValue( disk.usagePercent );
            }
        }

        const QList<QString> existingNames = m_diskItems.keys();
        for ( const QString& name : existingNames )
        {
            if ( !visibleDisks.contains( name ) )
            {
                ExMultiProgressRingItem* item = m_diskItems.take( name );
                m_diskRing->removeItem( item );
            }
        }

        if ( snapshot.diskActivity >= 0.0 )
        {
            updateAlert(
                QStringLiteral( "disk-activity" ),
                snapshot.diskActivity >=
                    ( m_alertStates.value( QStringLiteral( "disk-activity" ) ) ? 85.0 : 90.0 ),
                tr( "磁盘高占用" ),
                tr( "最高磁盘活动率为 %1%。" ).arg( snapshot.diskActivity, 0, 'f', 0 ) );
        }
    }

    QString activityText;
    if ( !snapshot.performanceSampleReady )
    {
        activityText = tr( "正在采样活动率" );
    }
    else if ( snapshot.diskActivityAvailable )
    {
        activityText = tr( "最高活动率 %1%" ).arg( snapshot.diskActivity, 0, 'f', 1 );
    }
    else
    {
        activityText = tr( "活动率不可用" );
    }

    QString capacityText;
    if ( snapshot.disks.isEmpty() )
    {
        capacityText = tr( "没有检测到本地磁盘" );
    }
    else
    {
        quint64 totalBytes = 0;
        quint64 availableBytes = 0;
        for ( const SystemDiskSnapshot& disk : snapshot.disks )
        {
            totalBytes += disk.totalBytes;
            availableBytes += disk.availableBytes;
        }
        capacityText = tr( "%1 个卷 · 可用 %2 / %3" )
                           .arg( snapshot.disks.size() )
                           .arg( formatBytes( availableBytes ), formatBytes( totalBytes ) );
    }
    m_diskDetails->setText( activityText + QStringLiteral( " · " ) + capacityText );
}

void SystemResourcesWidget::updateAlert( const QString& key,
                                         bool active,
                                         const QString& title,
                                         const QString& description )
{
    const bool wasActive = m_alertStates.value( key, false );
    if ( active == wasActive )
    {
        return;
    }

    m_alertStates.insert( key, active );
    if ( active )
    {
        appendTimelineEvent( title, description, ExTimelineEvent::Warning );
    }
    else if ( wasActive )
    {
        appendTimelineEvent( tr( "%1已恢复" ).arg( title ),
                             tr( "资源使用率已回到正常范围。" ),
                             ExTimelineEvent::Completed );
    }
}

void SystemResourcesWidget::appendTimelineEvent( const QString& title,
                                                 const QString& description,
                                                 int status )
{
    m_timeline->addEvent( QDateTime::currentDateTime(),
                          title,
                          description,
                          static_cast<ExTimelineEvent::Status>( status ) );
    while ( m_timeline->events().size() > 30 )
    {
        m_timeline->removeEvent( m_timeline->events().constFirst() );
    }
}

int SystemResourcesWidget::samplingInterval() const
{
    return m_intervalCombo->currentData().toInt();
}
