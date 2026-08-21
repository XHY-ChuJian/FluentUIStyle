#include "androidintegration.h"

#include <QtCore/qcoreapplication_platform.h>
#include <QJniObject>

namespace AndroidIntegration {

int sdkVersion()
{
    return QNativeInterface::QAndroidApplication::sdkVersion();
}

SystemSnapshot readSystemSnapshot()
{
    SystemSnapshot snapshot;
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid())
        return snapshot;

    const QJniObject activityService = QJniObject::fromString(QStringLiteral("activity"));
    const QJniObject activityManager = context.callObjectMethod(
        "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;", activityService.object());
    if (activityManager.isValid())
    {
        const QJniObject memoryInfo("android/app/ActivityManager$MemoryInfo", "()V");
        if (memoryInfo.isValid())
        {
            activityManager.callMethod<void>(
                "getMemoryInfo", "(Landroid/app/ActivityManager$MemoryInfo;)V", memoryInfo.object());
            snapshot.totalMemoryBytes = memoryInfo.getField<jlong>("totalMem");
            snapshot.availableMemoryBytes = memoryInfo.getField<jlong>("availMem");
            snapshot.lowMemoryThresholdBytes = memoryInfo.getField<jlong>("threshold");
            snapshot.lowMemory = memoryInfo.getField<jboolean>("lowMemory");
        }
    }

    snapshot.totalReceivedBytes = QJniObject::callStaticMethod<jlong>(
        "android/net/TrafficStats", "getTotalRxBytes", "()J");
    snapshot.totalTransmittedBytes = QJniObject::callStaticMethod<jlong>(
        "android/net/TrafficStats", "getTotalTxBytes", "()J");

    const QJniObject batteryAction =
        QJniObject::fromString(QStringLiteral("android.intent.action.BATTERY_CHANGED"));
    const QJniObject batteryFilter(
        "android/content/IntentFilter", "(Ljava/lang/String;)V", batteryAction.object());
    const QJniObject batteryIntent = context.callObjectMethod(
        "registerReceiver",
        "(Landroid/content/BroadcastReceiver;Landroid/content/IntentFilter;)Landroid/content/Intent;",
        jobject(nullptr), batteryFilter.object());
    if (batteryIntent.isValid())
    {
        const auto intExtra = [&batteryIntent](const char *name, jint fallback = -1) {
            const QJniObject key = QJniObject::fromString(QString::fromLatin1(name));
            return batteryIntent.callMethod<jint>(
                "getIntExtra", "(Ljava/lang/String;I)I", key.object(), fallback);
        };
        const jint level = intExtra("level");
        const jint scale = intExtra("scale", 100);
        const jint status = intExtra("status");
        if (level >= 0 && scale > 0)
            snapshot.batteryLevel = qBound(0, qRound(100.0 * level / scale), 100);
        snapshot.charging = status == 2 || status == 5;
    }

    return snapshot;
}

void showToast(const QString &message)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([message] {
        const QJniObject context = QNativeInterface::QAndroidApplication::context();
        const QJniObject text = QJniObject::fromString(message);
        const QJniObject toast = QJniObject::callStaticObjectMethod(
            "android/widget/Toast",
            "makeText",
            "(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;",
            context.object(), text.object(), jint(0));
        if (toast.isValid())
            toast.callMethod<void>("show", "()V");
    });
}

void shareText(const QString &subject, const QString &text)
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([subject, text] {
        const QJniObject context = QNativeInterface::QAndroidApplication::context();
        const QJniObject action = QJniObject::fromString(QStringLiteral("android.intent.action.SEND"));
        QJniObject intent("android/content/Intent", "(Ljava/lang/String;)V", action.object());
        if (!intent.isValid())
            return;

        const QJniObject mimeType = QJniObject::fromString(QStringLiteral("text/plain"));
        intent.callObjectMethod(
            "setType", "(Ljava/lang/String;)Landroid/content/Intent;", mimeType.object());

        const QJniObject subjectKey =
            QJniObject::fromString(QStringLiteral("android.intent.extra.SUBJECT"));
        const QJniObject subjectValue = QJniObject::fromString(subject);
        intent.callObjectMethod(
            "putExtra",
            "(Ljava/lang/String;Ljava/lang/CharSequence;)Landroid/content/Intent;",
            subjectKey.object(), subjectValue.object());

        const QJniObject textKey =
            QJniObject::fromString(QStringLiteral("android.intent.extra.TEXT"));
        const QJniObject textValue = QJniObject::fromString(text);
        intent.callObjectMethod(
            "putExtra",
            "(Ljava/lang/String;Ljava/lang/CharSequence;)Landroid/content/Intent;",
            textKey.object(), textValue.object());

        const QJniObject chooserTitle = QJniObject::fromString(QStringLiteral("分享 Fluent Android Gallery"));
        const QJniObject chooser = QJniObject::callStaticObjectMethod(
            "android/content/Intent",
            "createChooser",
            "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;",
            intent.object(), chooserTitle.object());
        if (!chooser.isValid())
            return;

        if (!QNativeInterface::QAndroidApplication::isActivityContext())
        {
            constexpr jint flagActivityNewTask = 0x10000000;
            chooser.callObjectMethod(
                "addFlags", "(I)Landroid/content/Intent;", flagActivityNewTask);
        }
        context.callMethod<void>(
            "startActivity", "(Landroid/content/Intent;)V", chooser.object());
    });
}

void performHapticFeedback()
{
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
        const QJniObject context = QNativeInterface::QAndroidApplication::context();
        const QJniObject window = context.callObjectMethod(
            "getWindow", "()Landroid/view/Window;");
        if (!window.isValid())
            return;

        const QJniObject decorView = window.callObjectMethod(
            "getDecorView", "()Landroid/view/View;");
        if (!decorView.isValid())
            return;

        constexpr jint virtualKeyFeedback = 1;
        decorView.callMethod<jboolean>(
            "performHapticFeedback", "(I)Z", virtualKeyFeedback);
    });
}

} // namespace AndroidIntegration
