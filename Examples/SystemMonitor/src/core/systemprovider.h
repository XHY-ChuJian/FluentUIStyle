#pragma once

#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QVector>

struct DiskInfo
{
    QString rootPath;
    QString displayName;
    QString fileSystemType;
    qint64 totalBytes = 0;
    qint64 availableBytes = 0;
    qint64 usedBytes = 0;
    double usagePercent = 0.0;
    bool isReady = false;
};

struct NetworkAdapterInfo
{
    QString id;
    QString name;
    QString description;
    QString macAddress;
    QString ipAddress;
    bool isUp = false;
};

struct ProcessItem
{
    qint64 pid = 0;
    QString name;
    QString path;
    qint64 memoryWorkingSetBytes = 0;
    double cpuPercent = 0.0;
    int threadCount = 0;
};

struct SystemSnapshot
{
    // CPU
    double cpuUsageTotal = 0.0;
    QVector<double> cpuUsageCores;
    int logicalCoreCount = 1;
    int physicalCoreCount = 1;
    QString cpuName;
    QString cpuArchitecture;

    // Memory
    qint64 totalPhysicalMemory = 0;
    qint64 availablePhysicalMemory = 0;
    qint64 usedPhysicalMemory = 0;
    double memoryUsagePercent = 0.0;

    qint64 totalVirtualMemory = 0;
    qint64 availableVirtualMemory = 0;
    qint64 usedVirtualMemory = 0;
    double virtualMemoryUsagePercent = 0.0;

    // Disks
    QList<DiskInfo> disks;
    qint64 totalDiskSpace = 0;
    qint64 totalDiskUsed = 0;
    double totalDiskUsagePercent = 0.0;

    // Network
    QList<NetworkAdapterInfo> adapters;
    qint64 totalBytesReceived = 0;
    qint64 totalBytesSent = 0;
    double downloadSpeedBytesPerSec = 0.0;
    double uploadSpeedBytesPerSec = 0.0;

    // System Info
    QString osVersion;
    QString osKernel;
    QString hostName;
    QString userName;
    qint64 uptimeSeconds = 0;
    QDateTime bootTime;

    // Processes (可按需刷新)
    QList<ProcessItem> processList;
};

class SystemProvider : public QObject
{
    Q_OBJECT

public:
    static SystemProvider &instance();

    explicit SystemProvider(QObject *parent = nullptr);
    ~SystemProvider() override;

    void start(int intervalMs = 1000);
    void stop();
    void setInterval(int intervalMs);
    int interval() const { return m_intervalMs; }

    const SystemSnapshot &lastSnapshot() const { return m_lastSnapshot; }
    
    // 手动立即采集一次基础数据
    SystemSnapshot refreshSnapshot();

    // 刷新进程列表（进程遍历开销相对大一点，可按需单独刷新）
    QList<ProcessItem> refreshProcesses();

    // 终止进程
    bool killProcess(qint64 pid);

    // 静态格式化实用工具
    static QString formatBytes(qint64 bytes);
    static QString formatSpeed(double bytesPerSec);
    static QString formatUptime(qint64 seconds);

signals:
    void snapshotUpdated(const SystemSnapshot &snapshot);
    void processesUpdated(const QList<ProcessItem> &processes);

private slots:
    void onTimerTick();

private:
    void initStaticInfo();
    void sampleCpu();
    void sampleMemory();
    void sampleDisks();
    void sampleNetwork();
    void sampleUptime();

    int m_intervalMs = 1000;
    QTimer *m_timer = nullptr;
    SystemSnapshot m_lastSnapshot;

    // Windows / Linux 内部维护的历史计数器状态
    qint64 m_lastSampleTimestampMs = 0;
    qint64 m_prevTotalReceivedBytes = -1;
    qint64 m_prevTotalSentBytes = -1;

#ifdef Q_OS_WIN
    quint64 m_prevIdleTime = 0;
    quint64 m_prevKernelTime = 0;
    quint64 m_prevUserTime = 0;
    QVector<quint64> m_prevCoreIdleTime;
    QVector<quint64> m_prevCoreKernelTime;
    QVector<quint64> m_prevCoreUserTime;
#else
    quint64 m_prevLinuxTotalTime = 0;
    quint64 m_prevLinuxIdleTime = 0;
    QVector<quint64> m_prevLinuxCoreTotalTime;
    QVector<quint64> m_prevLinuxCoreIdleTime;
#endif
};
