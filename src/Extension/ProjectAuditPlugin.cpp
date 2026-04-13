#include "apex/extension/ProjectAuditPlugin.h"

#include "apex/core/MathTypes.h"

#include <memory>
#include <sstream>

namespace apex::extension
{
    namespace
    {
        class JointLimitMarginAuditPlugin final : public IProjectAuditPlugin
        {
        public:
            [[nodiscard]] std::string GetName() const override { return "JointLimitMarginAudit"; }

            [[nodiscard]] std::vector<PluginFinding> Audit(
                const apex::io::StudioProject& project,
                const apex::core::SerialRobotModel&,
                const apex::workcell::CollisionWorld&) const override
            {
                std::vector<PluginFinding> findings;
                constexpr double marginRad = 5.0 * apex::core::Pi() / 180.0;
                for (std::size_t i = 0; i < project.Joints.size(); ++i)
                {
                    const auto& joint = project.Joints[i];
                    const auto check = [&](double angle, const char* label)
                    {
                        const double marginToMin = angle - joint.MinAngleRad;
                        const double marginToMax = joint.MaxAngleRad - angle;
                        if (marginToMin < marginRad || marginToMax < marginRad)
                        {
                            std::ostringstream builder;
                            builder << label << " angle for joint '" << joint.Name << "' is within 5 degrees of the configured limit.";
                            findings.push_back({FindingSeverity::Warning, builder.str()});
                        }
                    };
                    if (i < project.Motion.Start.JointAnglesRad.size())
                    {
                        check(project.Motion.Start.JointAnglesRad[i], "Start");
                    }
                    if (i < project.Motion.Goal.JointAnglesRad.size())
                    {
                        check(project.Motion.Goal.JointAnglesRad[i], "Goal");
                    }
                }
                return findings;
            }
        };

        class ObstacleCoverageAuditPlugin final : public IProjectAuditPlugin
        {
        public:
            [[nodiscard]] std::string GetName() const override { return "ObstacleCoverageAudit"; }

            [[nodiscard]] std::vector<PluginFinding> Audit(
                const apex::io::StudioProject& project,
                const apex::core::SerialRobotModel&,
                const apex::workcell::CollisionWorld&) const override
            {
                std::vector<PluginFinding> findings;
                if (project.Obstacles.empty())
                {
                    findings.push_back({FindingSeverity::Warning, "Workcell does not define any obstacles. Industrial integration review should verify safety fences and fixtures."});
                }
                else if (project.Obstacles.size() >= 6)
                {
                    findings.push_back({FindingSeverity::Info, "Workcell includes a rich obstacle set. This is suitable for interview discussion around collision envelopes."});
                }
                return findings;
            }
        };

        class JobStructureAuditPlugin final : public IProjectAuditPlugin
        {
        public:
            [[nodiscard]] std::string GetName() const override { return "JobStructureAudit"; }

            [[nodiscard]] std::vector<PluginFinding> Audit(
                const apex::io::StudioProject& project,
                const apex::core::SerialRobotModel&,
                const apex::workcell::CollisionWorld&) const override
            {
                std::vector<PluginFinding> findings;
                if (project.Job.Empty())
                {
                    findings.push_back({FindingSeverity::Info, "Project uses a single motion request and does not yet define a multi-step production job."});
                    return findings;
                }
                if (project.Job.Waypoints.size() < 3)
                {
                    findings.push_back({FindingSeverity::Warning, "Robot job contains fewer than three waypoints. Real workcells usually define home, approach, process, and retreat states."});
                }
                if (project.Job.Segments.empty())
                {
                    findings.push_back({FindingSeverity::Error, "Robot job has waypoints but no segments."});
                }
                return findings;
            }
        };
    }

    std::vector<std::shared_ptr<const IProjectAuditPlugin>> ProjectAuditPluginRegistry::CreateBuiltInPlugins() const
    {
        std::vector<std::shared_ptr<const IProjectAuditPlugin>> plugins;
        plugins.push_back(std::make_shared<JointLimitMarginAuditPlugin>());
        plugins.push_back(std::make_shared<ObstacleCoverageAuditPlugin>());
        plugins.push_back(std::make_shared<JobStructureAuditPlugin>());
        return plugins;
    }

    std::vector<PluginFinding> ProjectAuditPluginRegistry::RunBuiltInAudit(
        const apex::io::StudioProject& project,
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world) const
    {
        std::vector<PluginFinding> findings;
        for (const auto& plugin : CreateBuiltInPlugins())
        {
            const auto pluginFindings = plugin->Audit(project, robot, world);
            findings.insert(findings.end(), pluginFindings.begin(), pluginFindings.end());
        }
        return findings;
    }

    std::string ToString(FindingSeverity severity) noexcept
    {
        switch (severity)
        {
        case FindingSeverity::Info: return "info";
        case FindingSeverity::Warning: return "warning";
        case FindingSeverity::Error: return "error";
        }
        return "unknown";
    }
}
