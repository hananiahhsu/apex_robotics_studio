#pragma once

#include "apex/core/SerialRobotModel.h"
#include "apex/io/StudioProject.h"
#include "apex/workcell/CollisionWorld.h"

#include <memory>
#include <string>
#include <vector>

namespace apex::extension
{
    enum class FindingSeverity
    {
        Info,
        Warning,
        Error
    };

    struct PluginFinding
    {
        FindingSeverity Severity = FindingSeverity::Info;
        std::string Message;
    };

    class IProjectAuditPlugin
    {
    public:
        virtual ~IProjectAuditPlugin() = default;
        [[nodiscard]] virtual std::string GetName() const = 0;
        [[nodiscard]] virtual std::vector<PluginFinding> Audit(
            const apex::io::StudioProject& project,
            const apex::core::SerialRobotModel& robot,
            const apex::workcell::CollisionWorld& world) const = 0;
    };

    class ProjectAuditPluginRegistry
    {
    public:
        [[nodiscard]] std::vector<std::shared_ptr<const IProjectAuditPlugin>> CreateBuiltInPlugins() const;
        [[nodiscard]] std::vector<PluginFinding> RunBuiltInAudit(
            const apex::io::StudioProject& project,
            const apex::core::SerialRobotModel& robot,
            const apex::workcell::CollisionWorld& world) const;
    };

    [[nodiscard]] std::string ToString(FindingSeverity severity) noexcept;
}
