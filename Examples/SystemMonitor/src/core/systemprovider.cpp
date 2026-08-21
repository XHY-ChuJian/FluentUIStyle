#include "systemprovider.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QNetworkInterface>
#include <QProcess>
#include <QSettings>
#include <QStorageInfo>
#include <QSysInfo>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QtGlobal>
#include <QtMath>

#ifdef Q_OS_WIN
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <tlhelp32.h>

struct SysProcessorPerfInfo {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
};

typedef NTSTATUS(WINAPI *pfnNtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);
#else
#include <signal.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#endif

SystemProvider &SystemProvider::instance()
{
    static SystemProvider s_instance;
    return s_instance;
}

SystemProvider::SystemProvider(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    initStaticInfo();

    connect(m_timer, &QTimer::timeout, this, &SystemProvider::onTimerTick);
}

SystemProvider::~SystemProvider()
{
    stop();
}

void SystemProvider::start(int intervalMs)
{
    setInterval(intervalMs);
    refreshSnapshot();
    if (!m_timer->isActive()) {
        m_timer->start(m_intervalMs);
    }
}

void SystemProvider::stop()
{
    if (m_timer && m_timer->isActive()) {
        m_timer->stop();
    }
}

void SystemProvider::setInterval(int intervalMs)
{
    m_intervalMs = qMax(200, intervalMs);
    if (m_timer && m_timer->isActive()) {
        m_timer->setInterval(m_intervalMs);
    }
}

void SystemProvider::initStaticInfo()
{
    m_lastSnapshot.logicalCoreCount = QThread::idealThreadCount();
    if (m_lastSnapshot.logicalCoreCount <= 0) {
        m_lastSnapshot.logicalCoreCount = 1;
    }
    m_lastSnapshot.physicalCoreCount = m_lastSnapshot.logicalCoreCount; // 初始占位

    m_lastSnapshot.osVersion = QSysInfo::prettyProductName();
    m_lastSnapshot.osKernel = QSysInfo::kernelType() + QStringLiteral(" ") + QSysInfo::kernelVersion();
    m_lastSnapshot.cpuArchitecture = QSysInfo::currentCpuArchitecture();
    m_lastSnapshot.hostName = QSysInfo::machineHostName();

#ifdef Q_OS_WIN
    // 从 Windows 注册表获取准确的 CPU 名称
    QSettings cpuSettings(QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
                          QSettings::NativeFormat);
    m_lastSnapshot.cpuName = cpuSettings.value(QStringLiteral("ProcessorNameString")).toString().trimmed();
    if (m_lastSnapshot.cpuName.isEmpty()) {
        m_lastSnapshot.cpuName = QSysInfo::buildCpuArchitecture() + QStringLiteral(" Processor");
    }

    // 获取当前登录用户名
    wchar_t userBuf[256] = {0};
    DWORD userBufSize = 256;
    if (GetUserNameW(userBuf, &userBufSize)) {
        m_lastSnapshot.userName = QString::fromWCharArray(userBuf);
    } else {
        m_lastSnapshot.userName = qgetenv("USERNAME");
    }

    m_lastSnapshot.cpuUsageCores.resize(m_lastSnapshot.logicalCoreCount);
    m_lastSnapshot.cpuUsageCores.fill(0.0);
    m_prevCoreIdleTime.resize(m_lastSnapshot.logicalCoreCount);
    m_prevCoreKernelTime.resize(m_lastSnapshot.logicalCoreCount);
    m_prevCoreUserTime.resize(m_lastSnapshot.logicalCoreCount);
    m_prevCoreIdleTime.fill(0);
    m_prevCoreKernelTime.fill(0);
    m_prevCoreUserTime.fill(0);
#else
    m_lastSnapshot.userName = qgetenv("USER");
    if (m_lastSnapshot.userName.isEmpty()) {
        m_lastSnapshot.userName = qgetenv("LOGNAME");
    }

    // 从 Linux /proc/cpuinfo 获取 CPU 名称
    QFile cpuInfoFile(QStringLiteral("/proc/cpuinfo"));
    if (cpuInfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&cpuInfoFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith(QStringLiteral("model name"), Qt::CaseInsensitive)) {
                int colonIdx = line.indexOf(QLatin1Char(':'));
                if (colonIdx != -1) {
                    m_lastSnapshot.cpuName = line.mid(colonIdx + 1).trimmed();
                    break;
                }
            }
        }
        cpuInfoFile.close();
    }
    if (m_lastSnapshot.cpuName.isEmpty()) {
        m_lastSnapshot.cpuName = QSysInfo::currentCpuArchitecture() + QStringLiteral(" Processor");
    }

    m_lastSnapshot.cpuUsageCores.resize(m_lastSnapshot.logicalCoreCount);
    m_lastSnapshot.cpuUsageCores.fill(0.0);
    m_prevLinuxCoreTotalTime.resize(m_lastSnapshot.logicalCoreCount);
    m_prevLinuxCoreIdleTime.resize(m_lastSnapshot.logicalCoreCount);
    m_prevLinuxCoreTotalTime.fill(0);
    m_prevLinuxCoreIdleTime.fill(0);
#endif

    m_lastSampleTimestampMs = QDateTime::currentMSecsSinceEpoch();
}

SystemSnapshot SystemProvider::refreshSnapshot()
{
    sampleCpu();
    sampleMemory();
    sampleDisks();
    sampleNetwork();
    sampleUptime();

    m_lastSampleTimestampMs = QDateTime::currentMSecsSinceEpoch();

    emit snapshotUpdated(m_lastSnapshot);
    return m_lastSnapshot;
}

void SystemProvider::onTimerTick()
{
    refreshSnapshot();
}

void SystemProvider::sampleCpu()
{
#ifdef Q_OS_WIN
    FILETIME idleTimeFt, kernelTimeFt, userTimeFt;
    if (GetSystemTimes(&idleTimeFt, &kernelTimeFt, &userTimeFt)) {
        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart = idleTimeFt.dwLowDateTime;
        idle.HighPart = idleTimeFt.dwHighDateTime;
        kernel.LowPart = kernelTimeFt.dwLowDateTime;
        kernel.HighPart = kernelTimeFt.dwHighDateTime;
        user.LowPart = userTimeFt.dwLowDateTime;
        user.HighPart = userTimeFt.dwHighDateTime;

        quint64 curIdle = idle.QuadPart;
        quint64 curKernel = kernel.QuadPart;
        quint64 curUser = user.QuadPart;

        if (m_prevIdleTime > 0 || m_prevKernelTime > 0 || m_prevUserTime > 0) {
            quint64 idleDiff = curIdle - m_prevIdleTime;
            quint64 kernelDiff = curKernel - m_prevKernelTime;
            quint64 userDiff = curUser - m_prevUserTime;
            quint64 totalSysDiff = kernelDiff + userDiff;

            if (totalSysDiff > 0) {
                // Windows 下 kernelDiff 已包含 idleDiff
                double usage = (double)(totalSysDiff - idleDiff) * 100.0 / (double)totalSysDiff;
                m_lastSnapshot.cpuUsageTotal = qBound(0.0, usage, 100.0);
            }
        }

        m_prevIdleTime = curIdle;
        m_prevKernelTime = curKernel;
        m_prevUserTime = curUser;
    }

    // 针对每个 CPU 核心进行独立采样
    static HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
    static pfnNtQuerySystemInformation pNtQuerySystemInformation =
        hNtDll ? (pfnNtQuerySystemInformation)GetProcAddress(hNtDll, "NtQuerySystemInformation") : nullptr;

    if (pNtQuerySystemInformation) {
        int coreCount = m_lastSnapshot.logicalCoreCount;
        QVector<SysProcessorPerfInfo> perfInfo(coreCount);
        ULONG returnLength = 0;
        NTSTATUS status = pNtQuerySystemInformation(
            8, // SystemProcessorPerformanceInformation
            perfInfo.data(),
            sizeof(SysProcessorPerfInfo) * coreCount,
            &returnLength
        );

        if (status == 0) { // STATUS_SUCCESS
            for (int i = 0; i < coreCount; ++i) {
                quint64 cIdle = (quint64)perfInfo[i].IdleTime.QuadPart;
                quint64 cKernel = (quint64)perfInfo[i].KernelTime.QuadPart;
                quint64 cUser = (quint64)perfInfo[i].UserTime.QuadPart;

                if (m_prevCoreIdleTime[i] > 0 || m_prevCoreKernelTime[i] > 0) {
                    quint64 idleDiff = cIdle - m_prevCoreIdleTime[i];
                    quint64 kernelDiff = cKernel - m_prevCoreKernelTime[i];
                    quint64 userDiff = cUser - m_prevCoreUserTime[i];
                    quint64 totalDiff = kernelDiff + userDiff;

                    if (totalDiff > 0) {
                        double coreUsage = (double)(totalDiff - idleDiff) * 100.0 / (double)totalDiff;
                        m_lastSnapshot.cpuUsageCores[i] = qBound(0.0, coreUsage, 100.0);
                    }
                }

                m_prevCoreIdleTime[i] = cIdle;
                m_prevCoreKernelTime[i] = cKernel;
                m_prevCoreUserTime[i] = cUser;
            }
        }
    } else {
        // Fallback: 若无法获取各核心，则各核心均分总CPU
        for (int i = 0; i < m_lastSnapshot.cpuUsageCores.size(); ++i) {
            m_lastSnapshot.cpuUsageCores[i] = m_lastSnapshot.cpuUsageTotal;
        }
    }

#else
    // Linux 解析 /proc/stat
    QFile statFile(QStringLiteral("/proc/stat"));
    if (statFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&statFile);
        int coreIndex = 0;
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith(QStringLiteral("cpu "))) {
                QStringList parts = line.simplified().split(QLatin1Char(' '));
                if (parts.size() >= 5) {
                    quint64 user = parts[1].toULongLong();
                    quint64 nice = parts[2].toULongLong();
                    quint64 system = parts[3].toULongLong();
                    quint64 idle = parts[4].toULongLong();
                    quint64 iowait = parts.size() > 5 ? parts[5].toULongLong() : 0;
                    quint64 irq = parts.size() > 6 ? parts[6].toULongLong() : 0;
                    quint64 softirq = parts.size() > 7 ? parts[7].toULongLong() : 0;

                    quint64 total = user + nice + system + idle + iowait + irq + softirq;
                    quint64 totalIdle = idle + iowait;

                    if (m_prevLinuxTotalTime > 0) {
                        quint64 diffTotal = total - m_prevLinuxTotalTime;
                        quint64 diffIdle = totalIdle - m_prevLinuxIdleTime;
                        if (diffTotal > 0) {
                            double usage = (double)(diffTotal - diffIdle) * 100.0 / (double)diffTotal;
                            m_lastSnapshot.cpuUsageTotal = qBound(0.0, usage, 100.0);
                        }
                    }
                    m_prevLinuxTotalTime = total;
                    m_prevLinuxIdleTime = totalIdle;
                }
            } else if (line.startsWith(QStringLiteral("cpu")) && coreIndex < m_lastSnapshot.cpuUsageCores.size()) {
                QStringList parts = line.simplified().split(QLatin1Char(' '));
                if (parts.size() >= 5) {
                    quint64 user = parts[1].toULongLong();
                    quint64 nice = parts[2].toULongLong();
                    quint64 system = parts[3].toULongLong();
                    quint64 idle = parts[4].toULongLong();
                    quint64 iowait = parts.size() > 5 ? parts[5].toULongLong() : 0;
                    quint64 irq = parts.size() > 6 ? parts[6].toULongLong() : 0;
                    quint64 softirq = parts.size() > 7 ? parts[7].toULongLong() : 0;

                    quint64 total = user + nice + system + idle + iowait + irq + softirq;
                    quint64 totalIdle = idle + iowait;

                    if (m_prevLinuxCoreTotalTime[coreIndex] > 0) {
                        quint64 diffTotal = total - m_prevLinuxCoreTotalTime[coreIndex];
                        quint64 diffIdle = totalIdle - m_prevLinuxCoreIdleTime[coreIndex];
                        if (diffTotal > 0) {
                            double usage = (double)(diffTotal - diffIdle) * 100.0 / (double)diffTotal;
                            m_lastSnapshot.cpuUsageCores[coreIndex] = qBound(0.0, usage, 100.0);
                        }
                    }
                    m_prevLinuxCoreTotalTime[coreIndex] = total;
                    m_prevLinuxCoreIdleTime[coreIndex] = totalIdle;
                    coreIndex++;
                }
            }
        }
        statFile.close();
    }
#endif
}

void SystemProvider::sampleMemory()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        m_lastSnapshot.totalPhysicalMemory = (qint64)memStatus.ullTotalPhys;
        m_lastSnapshot.availablePhysicalMemory = (qint64)memStatus.ullAvailPhys;
        m_lastSnapshot.usedPhysicalMemory = m_lastSnapshot.totalPhysicalMemory - m_lastSnapshot.availablePhysicalMemory;
        m_lastSnapshot.memoryUsagePercent = (double)memStatus.dwMemoryLoad;

        m_lastSnapshot.totalVirtualMemory = (qint64)memStatus.ullTotalPageFile;
        m_lastSnapshot.availableVirtualMemory = (qint64)memStatus.ullAvailPageFile;
        m_lastSnapshot.usedVirtualMemory = m_lastSnapshot.totalVirtualMemory - m_lastSnapshot.availableVirtualMemory;
        if (m_lastSnapshot.totalVirtualMemory > 0) {
            m_lastSnapshot.virtualMemoryUsagePercent =
                (double)m_lastSnapshot.usedVirtualMemory * 100.0 / (double)m_lastSnapshot.totalVirtualMemory;
        }
    }
#else
    // Linux 解析 /proc/meminfo
    QFile memInfoFile(QStringLiteral("/proc/meminfo"));
    if (memInfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&memInfoFile);
        qint64 totalKb = 0;
        qint64 availKb = 0;
        qint64 freeKb = 0;
        qint64 buffersKb = 0;
        qint64 cachedKb = 0;
        qint64 swapTotalKb = 0;
        qint64 swapFreeKb = 0;

        while (!in.atEnd()) {
            QString line = in.readLine();
            if (line.startsWith(QStringLiteral("MemTotal:"))) {
                totalKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toLongLong();
            } else if (line.startsWith(QStringLiteral("MemAvailable:"))) {
                availKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toLongLong();
            } else if (line.startsWith(QStringLiteral("MemFree:"))) {
                freeKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toLongLong();
            } else if (line.startsWith(QStringLiteral("Buffers:"))) {
                buffersKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toLongLong();
            } else if (line.startsWith(QStringLiteral("Cached:"))) {
                cachedKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toLongLong();
            } else if (line.startsWith(QStringLiteral("SwapTotal:"))) {
                swapTotalKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toLongLong();
            } else if (line.startsWith(QStringLiteral("SwapFree:"))) {
                swapFreeKb = line.split(QLatin1Char(' '), Qt::SkipEmptyParts).value(1).toLongLong();
            }
        }
        memInfoFile.close();

        if (availKb == 0) {
            availKb = freeKb + buffersKb + cachedKb;
        }

        m_lastSnapshot.totalPhysicalMemory = totalKb * 1024;
        m_lastSnapshot.availablePhysicalMemory = availKb * 1024;
        m_lastSnapshot.usedPhysicalMemory = m_lastSnapshot.totalPhysicalMemory - m_lastSnapshot.availablePhysicalMemory;
        if (m_lastSnapshot.totalPhysicalMemory > 0) {
            m_lastSnapshot.memoryUsagePercent =
                (double)m_lastSnapshot.usedPhysicalMemory * 100.0 / (double)m_lastSnapshot.totalPhysicalMemory;
        }

        m_lastSnapshot.totalVirtualMemory = swapTotalKb * 1024;
        m_lastSnapshot.availableVirtualMemory = swapFreeKb * 1024;
        m_lastSnapshot.usedVirtualMemory = m_lastSnapshot.totalVirtualMemory - m_lastSnapshot.availableVirtualMemory;
        if (m_lastSnapshot.totalVirtualMemory > 0) {
            m_lastSnapshot.virtualMemoryUsagePercent =
                (double)m_lastSnapshot.usedVirtualMemory * 100.0 / (double)m_lastSnapshot.totalVirtualMemory;
        }
    }
#endif
}

void SystemProvider::sampleDisks()
{
    m_lastSnapshot.disks.clear();
    m_lastSnapshot.totalDiskSpace = 0;
    m_lastSnapshot.totalDiskUsed = 0;

    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &vol : volumes) {
        if (!vol.isValid() || !vol.isReady() || vol.bytesTotal() <= 0) {
            continue;
        }

        // 过滤部分 Linux 虚拟文件系统
        QString fsType = QString::fromLatin1(vol.fileSystemType()).toLower();
        if (fsType == QStringLiteral("squashfs") || fsType == QStringLiteral("tmpfs") ||
            fsType == QStringLiteral("devtmpfs") || fsType == QStringLiteral("overlay")) {
            continue;
        }

        DiskInfo disk;
        disk.rootPath = vol.rootPath();
        disk.displayName = vol.displayName();
        if (disk.displayName.isEmpty()) {
            disk.displayName = disk.rootPath;
        }
        disk.fileSystemType = QString::fromLatin1(vol.fileSystemType());
        disk.totalBytes = vol.bytesTotal();
        disk.availableBytes = vol.bytesAvailable();
        disk.usedBytes = disk.totalBytes - disk.availableBytes;
        disk.isReady = vol.isReady();

        if (disk.totalBytes > 0) {
            disk.usagePercent = (double)disk.usedBytes * 100.0 / (double)disk.totalBytes;
        }

        m_lastSnapshot.totalDiskSpace += disk.totalBytes;
        m_lastSnapshot.totalDiskUsed += disk.usedBytes;
        m_lastSnapshot.disks.append(disk);
    }

    if (m_lastSnapshot.totalDiskSpace > 0) {
        m_lastSnapshot.totalDiskUsagePercent =
            (double)m_lastSnapshot.totalDiskUsed * 100.0 / (double)m_lastSnapshot.totalDiskSpace;
    }
}

void SystemProvider::sampleNetwork()
{
    qint64 totalRecv = 0;
    qint64 totalSent = 0;
    m_lastSnapshot.adapters.clear();

#ifdef Q_OS_WIN
    ULONG ulOutBufLen = sizeof(MIB_IFTABLE);
    PMIB_IFTABLE pIfTable = (PMIB_IFTABLE)malloc(ulOutBufLen);
    if (pIfTable) {
        if (GetIfTable(pIfTable, &ulOutBufLen, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
            free(pIfTable);
            pIfTable = (PMIB_IFTABLE)malloc(ulOutBufLen);
        }
        if (pIfTable && GetIfTable(pIfTable, &ulOutBufLen, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; ++i) {
                const MIB_IFROW &row = pIfTable->table[i];
                if (row.dwType == MIB_IF_TYPE_LOOPBACK) {
                    continue;
                }

                if (row.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL || row.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED) {
                    totalRecv += (qint64)row.dwInOctets;
                    totalSent += (qint64)row.dwOutOctets;
                }

                NetworkAdapterInfo adapter;
                adapter.id = QString::number(row.dwIndex);
                adapter.name = QString::fromLocal8Bit((const char *)row.bDescr, (int)row.dwDescrLen).trimmed();
                adapter.description = adapter.name;
                adapter.isUp = (row.dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL || row.dwOperStatus == MIB_IF_OPER_STATUS_CONNECTED);

                QStringList macParts;
                for (DWORD j = 0; j < row.dwPhysAddrLen; ++j) {
                    macParts << QStringLiteral("%1").arg(row.bPhysAddr[j], 2, 16, QLatin1Char('0')).toUpper();
                }
                adapter.macAddress = macParts.join(QLatin1Char(':'));

                m_lastSnapshot.adapters.append(adapter);
            }
        }
        if (pIfTable) {
            free(pIfTable);
        }
    }

    // 填充 IP 地址
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (auto &ad : m_lastSnapshot.adapters) {
        for (const auto &netIf : interfaces) {
            QString ifMac = netIf.hardwareAddress().replace(QLatin1Char('-'), QLatin1Char(':')).toUpper();
            if ((!ad.macAddress.isEmpty() && ifMac == ad.macAddress) || netIf.humanReadableName() == ad.name || netIf.name() == ad.name) {
                for (const auto &entry : netIf.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                        ad.ipAddress = entry.ip().toString();
                        break;
                    }
                }
                break;
            }
        }
    }
#else
    // Linux /proc/net/dev
    QFile netDevFile(QStringLiteral("/proc/net/dev"));
    if (netDevFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&netDevFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            int colonIdx = line.indexOf(QLatin1Char(':'));
            if (colonIdx != -1) {
                QString ifName = line.left(colonIdx).trimmed();
                if (ifName == QStringLiteral("lo")) {
                    continue;
                }
                QString rest = line.mid(colonIdx + 1).simplified();
                QStringList parts = rest.split(QLatin1Char(' '));
                if (parts.size() >= 9) {
                    qint64 rxBytes = parts[0].toLongLong();
                    qint64 txBytes = parts[8].toLongLong();
                    totalRecv += rxBytes;
                    totalSent += txBytes;

                    NetworkAdapterInfo adapter;
                    adapter.name = ifName;
                    adapter.description = QStringLiteral("Network Adapter ") + ifName;
                    adapter.isUp = true;
                    m_lastSnapshot.adapters.append(adapter);
                }
            }
        }
        netDevFile.close();
    }
#endif

    m_lastSnapshot.totalBytesReceived = totalRecv;
    m_lastSnapshot.totalBytesSent = totalSent;

    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    qint64 dtMs = nowMs - m_lastSampleTimestampMs;
    if (dtMs <= 0) {
        dtMs = m_intervalMs;
    }

    if (m_prevTotalReceivedBytes >= 0 && m_prevTotalSentBytes >= 0 && totalRecv >= m_prevTotalReceivedBytes) {
        double dtSec = (double)dtMs / 1000.0;
        if (dtSec > 0.0) {
            m_lastSnapshot.downloadSpeedBytesPerSec = (double)(totalRecv - m_prevTotalReceivedBytes) / dtSec;
            m_lastSnapshot.uploadSpeedBytesPerSec = (double)(totalSent - m_prevTotalSentBytes) / dtSec;
        }
    } else {
        m_lastSnapshot.downloadSpeedBytesPerSec = 0.0;
        m_lastSnapshot.uploadSpeedBytesPerSec = 0.0;
    }

    m_prevTotalReceivedBytes = totalRecv;
    m_prevTotalSentBytes = totalSent;
}

void SystemProvider::sampleUptime()
{
#ifdef Q_OS_WIN
    ULONGLONG tickMs = GetTickCount64();
    m_lastSnapshot.uptimeSeconds = (qint64)(tickMs / 1000);
#else
    struct sysinfo s_info;
    if (sysinfo(&s_info) == 0) {
        m_lastSnapshot.uptimeSeconds = s_info.uptime;
    }
#endif

    m_lastSnapshot.bootTime = QDateTime::currentDateTime().addSecs(-m_lastSnapshot.uptimeSeconds);
}

QList<ProcessItem> SystemProvider::refreshProcesses()
{
    QList<ProcessItem> list;

#ifdef Q_OS_WIN
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(hSnap, &pe)) {
            do {
                if (pe.th32ProcessID == 0) {
                    continue;
                }

                ProcessItem item;
                item.pid = pe.th32ProcessID;
                item.name = QString::fromWCharArray(pe.szExeFile);
                item.threadCount = pe.cntThreads;

                // 查询内存大小
                HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
                if (hProc) {
                    PROCESS_MEMORY_COUNTERS pmc;
                    if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc))) {
                        item.memoryWorkingSetBytes = (qint64)pmc.WorkingSetSize;
                    }
                    CloseHandle(hProc);
                }

                list.append(item);
            } while (Process32NextW(hSnap, &pe));
        }
        CloseHandle(hSnap);
    }
#else
    // Linux 解析 /proc/[pid]
    QDir procDir(QStringLiteral("/proc"));
    const QStringList pids = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &pidStr : pids) {
        bool ok = false;
        qint64 pid = pidStr.toLongLong(&ok);
        if (!ok || pid <= 0) {
            continue;
        }

        ProcessItem item;
        item.pid = pid;

        // /proc/[pid]/comm
        QFile commFile(QStringLiteral("/proc/%1/comm").arg(pidStr));
        if (commFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            item.name = commFile.readLine().trimmed();
            commFile.close();
        }

        // /proc/[pid]/statm
        QFile statmFile(QStringLiteral("/proc/%1/statm").arg(pidStr));
        if (statmFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QStringList statm = QString::fromLatin1(statmFile.readAll()).split(QLatin1Char(' '), Qt::SkipEmptyParts);
            if (statm.size() >= 2) {
                // Resident pages * pageSize
                long pageSize = sysconf(_SC_PAGESIZE);
                item.memoryWorkingSetBytes = statm[1].toLongLong() * pageSize;
            }
            statmFile.close();
        }

        if (!item.name.isEmpty()) {
            list.append(item);
        }
    }
#endif

    m_lastSnapshot.processList = list;
    emit processesUpdated(list);
    return list;
}

bool SystemProvider::killProcess(qint64 pid)
{
    if (pid <= 0) {
        return false;
    }

#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
    if (!hProcess) {
        return false;
    }
    BOOL ok = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return (ok != FALSE);
#else
    int res = kill((pid_t)pid, SIGTERM);
    if (res != 0) {
        res = kill((pid_t)pid, SIGKILL);
    }
    return (res == 0);
#endif
}

QString SystemProvider::formatBytes(qint64 bytes)
{
    if (bytes < 0) {
        return QStringLiteral("0 B");
    }
    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;
    const double tb = gb * 1024.0;

    double dBytes = (double)bytes;
    if (dBytes >= tb) {
        return QStringLiteral("%1 TB").arg(dBytes / tb, 0, 'f', 2);
    }
    if (dBytes >= gb) {
        return QStringLiteral("%1 GB").arg(dBytes / gb, 0, 'f', 1);
    }
    if (dBytes >= mb) {
        return QStringLiteral("%1 MB").arg(dBytes / mb, 0, 'f', 1);
    }
    if (dBytes >= kb) {
        return QStringLiteral("%1 KB").arg(dBytes / kb, 0, 'f', 1);
    }
    return QStringLiteral("%1 B").arg(bytes);
}

QString SystemProvider::formatSpeed(double bytesPerSec)
{
    if (bytesPerSec <= 0.0) {
        return QStringLiteral("0 B/s");
    }
    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;

    if (bytesPerSec >= gb) {
        return QStringLiteral("%1 GB/s").arg(bytesPerSec / gb, 0, 'f', 2);
    }
    if (bytesPerSec >= mb) {
        return QStringLiteral("%1 MB/s").arg(bytesPerSec / mb, 0, 'f', 1);
    }
    if (bytesPerSec >= kb) {
        return QStringLiteral("%1 KB/s").arg(bytesPerSec / kb, 0, 'f', 1);
    }
    return QStringLiteral("%1 B/s").arg(qRound(bytesPerSec));
}

QString SystemProvider::formatUptime(qint64 seconds)
{
    if (seconds <= 0) {
        return QStringLiteral("刚刚启动");
    }
    qint64 days = seconds / 86400;
    qint64 hours = (seconds % 86400) / 3600;
    qint64 mins = (seconds % 3600) / 60;
    qint64 secs = seconds % 60;

    if (days > 0) {
        return QStringLiteral("%1天 %2小时 %3分").arg(days).arg(hours).arg(mins);
    }
    if (hours > 0) {
        return QStringLiteral("%1小时 %2分 %3秒").arg(hours).arg(mins).arg(secs);
    }
    return QStringLiteral("%1分 %2秒").arg(mins).arg(secs);
}
