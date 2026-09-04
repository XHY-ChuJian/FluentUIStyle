#include "fluentui3creatorplugin.h"
#include "fluentui3style.h"

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QWidget>

namespace FluentUI3 {
namespace Internal {

FluentUI3CreatorPlugin::FluentUI3CreatorPlugin()
{
}

FluentUI3CreatorPlugin::~FluentUI3CreatorPlugin()
{
}

void FluentUI3CreatorPlugin::initialize()
{
    // 记录原有的样式指针
    m_originalStyle = QApplication::style();

    // 在插件初始化阶段立即设置 FluentUI3Style
    applyFluentStyle();
}

void FluentUI3CreatorPlugin::extensionsInitialized()
{
    // 所有扩展插件加载完成后，主窗口等 UI 结构已完全建立
    // 再次确认样式应用并刷新所有顶层组件与控件
    applyFluentStyle();
    refreshAllWidgets();

    qInfo() << "[FluentUI3CreatorPlugin] Successfully applied FluentUI3Style to Qt Creator.";
}

ExtensionSystem::IPlugin::ShutdownFlag FluentUI3CreatorPlugin::aboutToShutdown()
{
    // Qt Creator 即将退出时的清理
    return SynchronousShutdown;
}

void FluentUI3CreatorPlugin::applyFluentStyle()
{
    if (m_applied && QApplication::style()->inherits("FluentUI3Style")) {
        return;
    }

    // 实例化 FluentUI3Style 并将其作为 Qt Creator 全局应用样式
    FluentUI3Style *fluentStyle = new FluentUI3Style;
    QApplication::setStyle(fluentStyle);
    m_applied = true;
}

void FluentUI3CreatorPlugin::refreshAllWidgets()
{
    // 遍历当前所有顶层部件和子部件，通知样式发生变更
    const QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
    for (QWidget *topWidget : topLevelWidgets) {
        if (!topWidget)
            continue;

        // 发送 StyleChange 事件通知部件重新从 QStyle 获取布局度量与绘制规则
        QEvent event(QEvent::StyleChange);
        QApplication::sendEvent(topWidget, &event);

        const QList<QWidget *> allChildren = topWidget->findChildren<QWidget *>();
        for (QWidget *child : allChildren) {
            QEvent childEvent(QEvent::StyleChange);
            QApplication::sendEvent(child, &childEvent);
        }

        topWidget->update();
    }
}

} // namespace Internal
} // namespace FluentUI3
