#pragma once

#include <QString>
#include <QtGlobal>

namespace AndroidIntegration {

struct SystemSnapshot
{
    qint64 totalMemoryBytes = -1;
    qint64 availableMemoryBytes = -1;
    qint64 lowMemoryThresholdBytes = -1;
    qint64 totalReceivedBytes = -1;
    qint64 totalTransmittedBytes = -1;
    int batteryLevel = -1;
    bool lowMemory = false;
    bool charging = false;
};

int sdkVersion();
SystemSnapshot readSystemSnapshot();
void showToast(const QString &message);
void shareText(const QString &subject, const QString &text);
void performHapticFeedback();

} // namespace AndroidIntegration
