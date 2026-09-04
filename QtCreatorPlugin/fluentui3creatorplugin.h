#pragma once

#include <extensionsystem/iplugin.h>
#include <QPointer>
#include <QStyle>

namespace FluentUI3 {
namespace Internal {

class FluentUI3CreatorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "FluentUI3CreatorPlugin.json")

public:
    FluentUI3CreatorPlugin();
    ~FluentUI3CreatorPlugin() override;

    void initialize() override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void applyFluentStyle();
    void refreshAllWidgets();

private:
    QPointer<QStyle> m_originalStyle;
    bool m_applied = false;
};

} // namespace Internal
} // namespace FluentUI3
