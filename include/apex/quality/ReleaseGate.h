#pragma once

#include "apex/analysis/TrajectoryAnalyzer.h"
#include "apex/extension/ProjectAuditPlugin.h"
#include "apex/io/StudioProject.h"
#include "apex/simulation/JobSimulationEngine.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::quality
{
    enum class GateStatus
    {
        Pass,
        Hold,
        Fail
    };

    struct GateCheck
    {
        std::string Name;
        GateStatus Status = GateStatus::Pass;
        std::string Message;
    };

    struct GateEvaluation
    {
        GateStatus Status = GateStatus::Pass;
        std::vector<GateCheck> Checks;
        std::size_t ErrorFindingCount = 0;
        std::size_t WarningFindingCount = 0;
        double PeakTcpSpeedMetersPerSecond = 0.0;
        double AverageTcpSpeedMetersPerSecond = 0.0;
        bool CollisionFree = true;
    };

    class ReleaseGateEvaluator
    {
    public:
        [[nodiscard]] GateEvaluation Evaluate(
            const apex::io::StudioProject& project,
            const apex::simulation::JobSimulationResult& simulation,
            const std::vector<apex::extension::PluginFinding>& findings) const;

        [[nodiscard]] std::string BuildHtmlReport(
            const apex::io::StudioProject& project,
            const GateEvaluation& evaluation) const;

        void SaveHtmlReport(
            const apex::io::StudioProject& project,
            const GateEvaluation& evaluation,
            const std::filesystem::path& filePath) const;
    };

    [[nodiscard]] std::string ToString(GateStatus status) noexcept;
}
