#include "apex/quality/ReleaseGate.h"

#include <fstream>
#include <sstream>

namespace apex::quality
{
    namespace
    {
        GateStatus MaxStatus(GateStatus lhs, GateStatus rhs)
        {
            return static_cast<int>(lhs) >= static_cast<int>(rhs) ? lhs : rhs;
        }
    }

    std::string ToString(GateStatus status) noexcept
    {
        switch (status)
        {
        case GateStatus::Pass: return "PASS";
        case GateStatus::Hold: return "HOLD";
        case GateStatus::Fail: return "FAIL";
        }
        return "UNKNOWN";
    }

    GateEvaluation ReleaseGateEvaluator::Evaluate(
        const apex::io::StudioProject& project,
        const apex::simulation::JobSimulationResult& simulation,
        const std::vector<apex::extension::PluginFinding>& findings) const
    {
        GateEvaluation evaluation;
        evaluation.CollisionFree = simulation.CollisionFree;

        for (const auto& segment : simulation.SegmentResults)
        {
            evaluation.PeakTcpSpeedMetersPerSecond = std::max(
                evaluation.PeakTcpSpeedMetersPerSecond,
                segment.Quality.PeakTcpSpeedMetersPerSecond);
            evaluation.AverageTcpSpeedMetersPerSecond = std::max(
                evaluation.AverageTcpSpeedMetersPerSecond,
                segment.Quality.AverageTcpSpeedMetersPerSecond);
        }

        for (const auto& finding : findings)
        {
            if (finding.Severity == apex::extension::FindingSeverity::Error)
            {
                ++evaluation.ErrorFindingCount;
            }
            else if (finding.Severity == apex::extension::FindingSeverity::Warning)
            {
                ++evaluation.WarningFindingCount;
            }
        }

        auto addCheck = [&](const std::string& name, GateStatus status, const std::string& message)
        {
            evaluation.Checks.push_back({ name, status, message });
            evaluation.Status = MaxStatus(evaluation.Status, status);
        };

        if (project.QualityGate.RequireCollisionFree)
        {
            addCheck(
                "CollisionFree",
                simulation.CollisionFree ? GateStatus::Pass : GateStatus::Fail,
                simulation.CollisionFree ? "No collisions detected across all segments."
                                        : "At least one segment reported collisions.");
        }

        addCheck(
            "ErrorFindings",
            evaluation.ErrorFindingCount <= project.QualityGate.MaxErrorFindings ? GateStatus::Pass : GateStatus::Fail,
            "Errors=" + std::to_string(evaluation.ErrorFindingCount) +
            ", allowed=" + std::to_string(project.QualityGate.MaxErrorFindings));

        addCheck(
            "WarningFindings",
            evaluation.WarningFindingCount <= project.QualityGate.MaxWarningFindings ? GateStatus::Pass : GateStatus::Hold,
            "Warnings=" + std::to_string(evaluation.WarningFindingCount) +
            ", allowed=" + std::to_string(project.QualityGate.MaxWarningFindings));

        addCheck(
            "PeakTcpSpeed",
            evaluation.PeakTcpSpeedMetersPerSecond <= project.QualityGate.MaxPeakTcpSpeedMetersPerSecond ? GateStatus::Pass : GateStatus::Hold,
            "Peak TCP speed=" + std::to_string(evaluation.PeakTcpSpeedMetersPerSecond) +
            " m/s, limit=" + std::to_string(project.QualityGate.MaxPeakTcpSpeedMetersPerSecond) + " m/s");

        addCheck(
            "AverageTcpSpeed",
            evaluation.AverageTcpSpeedMetersPerSecond <= project.QualityGate.MaxAverageTcpSpeedMetersPerSecond ? GateStatus::Pass : GateStatus::Hold,
            "Average TCP speed=" + std::to_string(evaluation.AverageTcpSpeedMetersPerSecond) +
            " m/s, limit=" + std::to_string(project.QualityGate.MaxAverageTcpSpeedMetersPerSecond) + " m/s");

        return evaluation;
    }

    std::string ReleaseGateEvaluator::BuildHtmlReport(
        const apex::io::StudioProject& project,
        const GateEvaluation& evaluation) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Release Gate</title>"
             << "<style>body{font-family:Arial,sans-serif;margin:24px;}table{border-collapse:collapse;width:100%;}"
             << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}th{background:#f5f5f5;}"
             << ".PASS{color:#0a7d18;font-weight:bold;}.HOLD{color:#b36b00;font-weight:bold;}.FAIL{color:#b00020;font-weight:bold;}"
             << "</style></head><body>";
        html << "<h1>ApexRoboticsStudio Release Gate</h1>";
        html << "<p><strong>Project:</strong> " << project.ProjectName << "<br>";
        html << "<strong>Cell:</strong> " << project.Metadata.CellName << "<br>";
        html << "<strong>Process family:</strong> " << project.Metadata.ProcessFamily << "<br>";
        html << "<strong>Owner:</strong> " << project.Metadata.Owner << "</p>";
        html << "<h2>Overall Status: <span class=\"" << ToString(evaluation.Status) << "\">" << ToString(evaluation.Status) << "</span></h2>";
        html << "<p><strong>Peak TCP speed:</strong> " << evaluation.PeakTcpSpeedMetersPerSecond
             << " m/s<br><strong>Average TCP speed:</strong> " << evaluation.AverageTcpSpeedMetersPerSecond
             << " m/s<br><strong>Error findings:</strong> " << evaluation.ErrorFindingCount
             << "<br><strong>Warning findings:</strong> " << evaluation.WarningFindingCount << "</p>";
        html << "<table><thead><tr><th>Check</th><th>Status</th><th>Message</th></tr></thead><tbody>";
        for (const auto& check : evaluation.Checks)
        {
            html << "<tr><td>" << check.Name << "</td><td class=\"" << ToString(check.Status) << "\">"
                 << ToString(check.Status) << "</td><td>" << check.Message << "</td></tr>";
        }
        html << "</tbody></table></body></html>";
        return html.str();
    }

    void ReleaseGateEvaluator::SaveHtmlReport(
        const apex::io::StudioProject& project,
        const GateEvaluation& evaluation,
        const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write gate report: " + filePath.string());
        }
        stream << BuildHtmlReport(project, evaluation);
    }
}
