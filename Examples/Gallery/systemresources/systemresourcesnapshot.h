#pragma once

#include <QDateTime>
#include <QList>
#include <QString>
#include <QtGlobal>

struct SystemDiskSnapshot
{
    QString name;
    quint64 totalBytes = 0;
    quint64 availableBytes = 0;
    qreal usedPercent = 0.0;
};

struct SystemDiskActivitySnapshot
{
    QString name;
    qreal usagePercent = 0.0;
};

struct SystemNetworkSnapshot
{
    QString name;
    quint64 receivedBytesPerSecond = 0;
    quint64 sentBytesPerSecond = 0;
    bool connected = false;
    bool rateAvailable = false;
};

struct SystemResourceSnapshot
{
    QDateTime timestamp;
    qreal cpuUsage = -1.0;
    quint64 totalMemoryBytes = 0;
    quint64 availableMemoryBytes = 0;
    qreal memoryUsage = 0.0;
    QList<SystemDiskSnapshot> disks;
    QList<SystemDiskActivitySnapshot> diskActivities;
    qreal diskActivity = -1.0;
    qreal gpuUsage = -1.0;
    QString gpuEngineName;
    bool performanceSampleReady = false;
    bool diskActivityAvailable = false;
    bool gpuAvailable = false;
    SystemNetworkSnapshot network;
    quint64 uptimeSeconds = 0;
    bool available = false;
};
