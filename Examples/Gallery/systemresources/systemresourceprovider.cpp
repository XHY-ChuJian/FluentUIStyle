#ifdef _WIN32
#if !defined( WINVER ) || WINVER < 0x0601
#undef WINVER
#define WINVER 0x0601
#endif
#if !defined( _WIN32_WINNT ) || _WIN32_WINNT < 0x0601
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#if !defined( NTDDI_VERSION ) || NTDDI_VERSION < 0x06010000
#undef NTDDI_VERSION
#define NTDDI_VERSION 0x06010000
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <pdh.h>
#include <pdhmsg.h>
#endif

#include "systemresourceprovider.h"

#include <QDateTime>
#include <QHash>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QtMath>

#include <algorithm>
#include <cwchar>
#include <limits>

struct SystemResourceNativeCounters
{
#ifdef Q_OS_WIN
    PDH_HQUERY query = nullptr;
    PDH_HCOUNTER diskActivity = nullptr;
    PDH_HCOUNTER gpuUsage = nullptr;
    bool primed = false;
#endif
};

namespace
{
#ifdef Q_OS_WIN
struct FormattedCounterValue
{
    QString name;
    qreal value = 0.0;
};

QList<FormattedCounterValue> formattedCounterValues( PDH_HCOUNTER counter )
{
    if ( !counter )
    {
        return {};
    }

    DWORD bufferSize = 0;
    DWORD itemCount = 0;
    PDH_STATUS status = PdhGetFormattedCounterArrayW( counter,
                                                       PDH_FMT_DOUBLE,
                                                       &bufferSize,
                                                       &itemCount,
                                                       nullptr );
    if ( status != PDH_MORE_DATA || bufferSize == 0 ||
         bufferSize > static_cast<DWORD>( std::numeric_limits<int>::max() ) )
    {
        return {};
    }

    QVector<uchar> buffer( static_cast<int>( bufferSize ) );
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>( buffer.data() );
    status = PdhGetFormattedCounterArrayW( counter,
                                            PDH_FMT_DOUBLE,
                                            &bufferSize,
                                            &itemCount,
                                            items );
    if ( status != ERROR_SUCCESS )
    {
        return {};
    }

    QList<FormattedCounterValue> values;
    values.reserve( static_cast<int>( itemCount ) );
    for ( DWORD index = 0; index < itemCount; ++index )
    {
        const PDH_FMT_COUNTERVALUE& formattedValue = items[index].FmtValue;
        if ( formattedValue.CStatus != PDH_CSTATUS_VALID_DATA &&
             formattedValue.CStatus != PDH_CSTATUS_NEW_DATA )
        {
            continue;
        }
        if ( qIsFinite( formattedValue.doubleValue ) )
        {
            values.append( {QString::fromWCharArray( items[index].szName ),
                             formattedValue.doubleValue} );
        }
    }
    return values;
}

quint64 fileTimeValue( const FILETIME& value )
{
    ULARGE_INTEGER result;
    result.LowPart = value.dwLowDateTime;
    result.HighPart = value.dwHighDateTime;
    return result.QuadPart;
}

qreal percentage( quint64 used, quint64 total )
{
    if ( total == 0 )
    {
        return 0.0;
    }
    return qBound( 0.0, static_cast<qreal>( used ) * 100.0 / static_cast<qreal>( total ), 100.0 );
}
#endif
}

SystemResourceProvider::SystemResourceProvider( QObject* parent )
    : QObject( parent )
    , m_timer( new QTimer( this ) )
    , m_nativeCounters( new SystemResourceNativeCounters )
{
    m_timer->setInterval( 1000 );
    m_timer->setTimerType( Qt::CoarseTimer );
    connect( m_timer, &QTimer::timeout, this, &SystemResourceProvider::collect );

#ifdef Q_OS_WIN
    if ( PdhOpenQueryW( nullptr, 0, &m_nativeCounters->query ) == ERROR_SUCCESS )
    {
        PdhAddEnglishCounterW( m_nativeCounters->query,
                               L"\\PhysicalDisk(*)\\% Disk Time",
                               0,
                               &m_nativeCounters->diskActivity );
        PdhAddEnglishCounterW( m_nativeCounters->query,
                               L"\\GPU Engine(*)\\Utilization Percentage",
                               0,
                               &m_nativeCounters->gpuUsage );
        if ( !m_nativeCounters->diskActivity && !m_nativeCounters->gpuUsage )
        {
            PdhCloseQuery( m_nativeCounters->query );
            m_nativeCounters->query = nullptr;
        }
    }
#endif
}

SystemResourceProvider::~SystemResourceProvider()
{
#ifdef Q_OS_WIN
    if ( m_nativeCounters->query )
    {
        PdhCloseQuery( m_nativeCounters->query );
    }
#endif
    delete m_nativeCounters;
}

void SystemResourceProvider::start( int intervalMilliseconds )
{
    setInterval( intervalMilliseconds );
    resetSamplingState();
    collect();
    m_timer->start();
}

void SystemResourceProvider::stop()
{
    m_timer->stop();
}

void SystemResourceProvider::setInterval( int intervalMilliseconds )
{
    m_timer->setInterval( qBound( 250, intervalMilliseconds, 60000 ) );
}

bool SystemResourceProvider::isRunning() const
{
    return m_timer->isActive();
}

void SystemResourceProvider::resetSamplingState()
{
    m_previousIdleTime = 0;
    m_previousKernelTime = 0;
    m_previousUserTime = 0;
    m_previousReceivedBytes = 0;
    m_previousSentBytes = 0;
    m_previousNetworkTimestamp = 0;
    m_previousNetworkKey.clear();
#ifdef Q_OS_WIN
    m_nativeCounters->primed = false;
#endif
}

void SystemResourceProvider::collect()
{
    SystemResourceSnapshot snapshot;
    snapshot.timestamp = QDateTime::currentDateTime();

#ifdef Q_OS_WIN
    snapshot.available = true;
    snapshot.performanceSampleReady = !m_nativeCounters->query;
    snapshot.uptimeSeconds = GetTickCount64() / 1000;

    FILETIME idleTime;
    FILETIME kernelTime;
    FILETIME userTime;
    if ( GetSystemTimes( &idleTime, &kernelTime, &userTime ) )
    {
        const quint64 idle = fileTimeValue( idleTime );
        const quint64 kernel = fileTimeValue( kernelTime );
        const quint64 user = fileTimeValue( userTime );
        if ( m_previousKernelTime != 0 && kernel >= m_previousKernelTime &&
             user >= m_previousUserTime && idle >= m_previousIdleTime )
        {
            const quint64 idleDelta = idle - m_previousIdleTime;
            const quint64 totalDelta = kernel - m_previousKernelTime + user - m_previousUserTime;
            snapshot.cpuUsage = totalDelta > 0
                ? percentage( totalDelta - qMin( idleDelta, totalDelta ), totalDelta )
                : 0.0;
        }
        m_previousIdleTime = idle;
        m_previousKernelTime = kernel;
        m_previousUserTime = user;
    }

    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof( memoryStatus );
    if ( GlobalMemoryStatusEx( &memoryStatus ) )
    {
        snapshot.totalMemoryBytes = memoryStatus.ullTotalPhys;
        snapshot.availableMemoryBytes = memoryStatus.ullAvailPhys;
        snapshot.memoryUsage = percentage( snapshot.totalMemoryBytes - snapshot.availableMemoryBytes,
                                           snapshot.totalMemoryBytes );
    }

    const DWORD driveBufferLength = GetLogicalDriveStringsW( 0, nullptr );
    if ( driveBufferLength > 0 )
    {
        QVector<wchar_t> driveBuffer( static_cast<int>( driveBufferLength ) + 1 );
        if ( GetLogicalDriveStringsW( driveBufferLength, driveBuffer.data() ) > 0 )
        {
            const wchar_t* drive = driveBuffer.constData();
            while ( *drive != L'\0' )
            {
                if ( GetDriveTypeW( drive ) == DRIVE_FIXED )
                {
                    ULARGE_INTEGER availableBytes;
                    ULARGE_INTEGER totalBytes;
                    ULARGE_INTEGER totalFreeBytes;
                    if ( GetDiskFreeSpaceExW( drive, &availableBytes, &totalBytes, &totalFreeBytes ) )
                    {
                        SystemDiskSnapshot disk;
                        disk.name = QString::fromWCharArray( drive ).left( 2 );
                        disk.totalBytes = totalBytes.QuadPart;
                        disk.availableBytes = totalFreeBytes.QuadPart;
                        disk.usedPercent = percentage( disk.totalBytes - disk.availableBytes,
                                                       disk.totalBytes );
                        snapshot.disks.append( disk );
                    }
                }
                drive += std::wcslen( drive ) + 1;
            }
        }
    }

    MIB_IF_TABLE2* interfaceTable = nullptr;
    if ( GetIfTable2( &interfaceTable ) == NO_ERROR && interfaceTable )
    {
        quint64 receivedBytes = 0;
        quint64 sentBytes = 0;
        QStringList interfaceNames;
        QStringList interfaceKeys;
        int activeInterfaceCount = 0;

        for ( ULONG index = 0; index < interfaceTable->NumEntries; ++index )
        {
            const MIB_IF_ROW2& row = interfaceTable->Table[index];
            if ( row.OperStatus != IfOperStatusUp ||
                 row.MediaConnectState != MediaConnectStateConnected ||
                 row.Type == IF_TYPE_SOFTWARE_LOOPBACK ||
                 row.TunnelType != TUNNEL_TYPE_NONE ||
                 row.InterfaceAndOperStatusFlags.FilterInterface )
            {
                continue;
            }

            receivedBytes += row.InOctets;
            sentBytes += row.OutOctets;
            ++activeInterfaceCount;
            interfaceKeys.append( QString::number( row.InterfaceIndex ) );
            const QString alias = QString::fromWCharArray( row.Alias ).trimmed();
            if ( !alias.isEmpty() )
            {
                interfaceNames.append( alias );
            }
        }
        FreeMibTable( interfaceTable );

        interfaceKeys.sort();
        const QString networkKey = interfaceKeys.join( QLatin1Char( ';' ) );
        snapshot.network.connected = activeInterfaceCount > 0;
        snapshot.network.name = activeInterfaceCount == 1 && interfaceNames.size() == 1
            ? interfaceNames.constFirst()
            : tr( "%1 个活动适配器" ).arg( activeInterfaceCount );

        const qint64 now = snapshot.timestamp.toMSecsSinceEpoch();
        const qint64 elapsedMilliseconds = now - m_previousNetworkTimestamp;
        if ( m_previousNetworkTimestamp > 0 && m_previousNetworkKey == networkKey &&
             elapsedMilliseconds > 0 &&
             receivedBytes >= m_previousReceivedBytes && sentBytes >= m_previousSentBytes )
        {
            snapshot.network.rateAvailable = true;
            const qreal elapsedSeconds = static_cast<qreal>( elapsedMilliseconds ) / 1000.0;
            const quint64 receivedDelta = receivedBytes - m_previousReceivedBytes;
            const quint64 sentDelta = sentBytes - m_previousSentBytes;
            snapshot.network.receivedBytesPerSecond =
                static_cast<quint64>( static_cast<qreal>( receivedDelta ) / elapsedSeconds );
            snapshot.network.sentBytesPerSecond =
                static_cast<quint64>( static_cast<qreal>( sentDelta ) / elapsedSeconds );
        }
        m_previousReceivedBytes = receivedBytes;
        m_previousSentBytes = sentBytes;
        m_previousNetworkTimestamp = now;
        m_previousNetworkKey = networkKey;
    }

    if ( m_nativeCounters->query && PdhCollectQueryData( m_nativeCounters->query ) == ERROR_SUCCESS )
    {
        if ( m_nativeCounters->primed )
        {
            snapshot.performanceSampleReady = true;
            qreal totalDiskActivity = -1.0;
            const QList<FormattedCounterValue> diskValues =
                formattedCounterValues( m_nativeCounters->diskActivity );
            for ( const FormattedCounterValue& value : diskValues )
            {
                const qreal usage = qBound( 0.0, value.value, 100.0 );
                if ( value.name.compare( QStringLiteral( "_Total" ), Qt::CaseInsensitive ) == 0 )
                {
                    totalDiskActivity = usage;
                    continue;
                }
                snapshot.diskActivities.append( {value.name, usage} );
                snapshot.diskActivity = qMax( snapshot.diskActivity, usage );
            }
            if ( snapshot.diskActivities.isEmpty() && totalDiskActivity >= 0.0 )
            {
                snapshot.diskActivities.append( {QStringLiteral( "_Total" ), totalDiskActivity} );
                snapshot.diskActivity = totalDiskActivity;
            }
            std::sort( snapshot.diskActivities.begin(),
                       snapshot.diskActivities.end(),
                       []( const SystemDiskActivitySnapshot& left,
                           const SystemDiskActivitySnapshot& right )
                       {
                           bool leftNumberValid = false;
                           bool rightNumberValid = false;
                           const int leftNumber = left.name.section( QLatin1Char( ' ' ), 0, 0 )
                                                      .toInt( &leftNumberValid );
                           const int rightNumber = right.name.section( QLatin1Char( ' ' ), 0, 0 )
                                                        .toInt( &rightNumberValid );
                           if ( leftNumberValid && rightNumberValid && leftNumber != rightNumber )
                           {
                               return leftNumber < rightNumber;
                           }
                           return left.name.compare( right.name, Qt::CaseInsensitive ) < 0;
                       } );
            snapshot.diskActivityAvailable = !snapshot.diskActivities.isEmpty();

            QHash<QString, qreal> usageByEngine;
            const QList<FormattedCounterValue> gpuValues =
                formattedCounterValues( m_nativeCounters->gpuUsage );
            snapshot.gpuAvailable = !gpuValues.isEmpty();
            for ( const FormattedCounterValue& value : gpuValues )
            {
                const int luidIndex = value.name.indexOf( QStringLiteral( "luid_" ),
                                                          0,
                                                          Qt::CaseInsensitive );
                QString engineKey = luidIndex >= 0 ? value.name.mid( luidIndex ) : value.name;
                const int duplicateSuffix = engineKey.lastIndexOf( QLatin1Char( '#' ) );
                bool hasNumericSuffix = false;
                if ( duplicateSuffix >= 0 )
                {
                    engineKey.mid( duplicateSuffix + 1 ).toInt( &hasNumericSuffix );
                }
                if ( hasNumericSuffix )
                {
                    engineKey.truncate( duplicateSuffix );
                }
                usageByEngine[engineKey] += qMax( 0.0, value.value );
            }
            for ( auto iterator = usageByEngine.constBegin(); iterator != usageByEngine.constEnd(); ++iterator )
            {
                const qreal usage = qBound( 0.0, iterator.value(), 100.0 );
                if ( usage > snapshot.gpuUsage )
                {
                    snapshot.gpuUsage = usage;
                    const int engineTypeIndex = iterator.key().indexOf(
                        QStringLiteral( "engtype_" ), 0, Qt::CaseInsensitive );
                    snapshot.gpuEngineName = engineTypeIndex >= 0
                        ? iterator.key().mid( engineTypeIndex + 8 )
                        : QString();
                }
            }
        }
        m_nativeCounters->primed = true;
    }
    else if ( m_nativeCounters->query )
    {
        snapshot.performanceSampleReady = true;
        m_nativeCounters->primed = false;
    }
#endif

    emit snapshotReady( snapshot );
}
