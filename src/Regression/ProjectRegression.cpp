#include "apex/regression/ProjectRegression.h"

#include <fstream>
#include <sstream>

namespace apex::regression
{
    namespace
    {
        int GateRank(apex::quality::GateStatus status) noexcept
        {
            switch (status)
            {
            case apex::quality::GateStatus::Pass: return 0;
            case apex::quality::GateStatus::Hold: return 1;
            case apex::quality::GateStatus::Fail: return 2;
            }
            return 2;
        }
    }

    std::string ToString(RegressionStatus status) noexcept
    {
        switch (status)
        {
        case RegressionStatus::Improved: return "IMPROVED";
        case RegressionStatus::Equivalent: return "EQUIVALENT";
        case RegressionStatus::Regressed: return "REGRESSED";
        }
        return "UNKNOWN";
    }

    ProjectRegressionResult ProjectRegressionAnalyzer::Evaluate(
        const apex::simulation::JobSimulationResult& baselineSimulation,
        const apex::quality::GateEvaluation& baselineGate,
        const apex::simulation::JobSimulationResult& candidateSimulation,
        const apex::quality::GateEvaluation& candidateGate) const
    {
        ProjectRegressionResult result;
        result.BaselineGateStatus = baselineGate.Status;
        result.CandidateGateStatus = candidateGate.Status;
        result.BaselineDurationSeconds = baselineSimulation.TotalDurationSeconds;
        result.CandidateDurationSeconds = candidateSimulation.TotalDurationSeconds;
        result.DurationDeltaSeconds = candidateSimulation.TotalDurationSeconds - baselineSimulation.TotalDurationSeconds;
        result.BaselinePathLengthMeters = baselineSimulation.TotalPathLengthMeters;
        result.CandidatePathLengthMeters = candidateSimulation.TotalPathLengthMeters;
        result.PathDeltaMeters = candidateSimulation.TotalPathLengthMeters - baselineSimulation.TotalPathLengthMeters;
        result.BaselineErrorFindings = baselineGate.ErrorFindingCount;
        result.CandidateErrorFindings = candidateGate.ErrorFindingCount;
        result.BaselineWarningFindings = baselineGate.WarningFindingCount;
        result.CandidateWarningFindings = candidateGate.WarningFindingCount;
        result.BaselineCollisionFree = baselineGate.CollisionFree;
        result.CandidateCollisionFree = candidateGate.CollisionFree;

        const int baselineRank = GateRank(baselineGate.Status);
        const int candidateRank = GateRank(candidateGate.Status);
        bool improved = false;
        bool regressed = false;

        if (candidateRank > baselineRank)
        {
            regressed = true;
            result.Notes.push_back("Candidate gate status is worse than baseline.");
        }
        else if (candidateRank < baselineRank)
        {
            improved = true;
            result.Notes.push_back("Candidate gate status is better than baseline.");
        }

        if (baselineGate.CollisionFree && !candidateGate.CollisionFree)
        {
            regressed = true;
            result.Notes.push_back("Candidate introduced collisions compared with the baseline.");
        }
        else if (!baselineGate.CollisionFree && candidateGate.CollisionFree)
        {
            improved = true;
            result.Notes.push_back("Candidate removed collisions that existed in the baseline.");
        }

        if (candidateGate.ErrorFindingCount > baselineGate.ErrorFindingCount)
        {
            regressed = true;
            result.Notes.push_back("Candidate increased error findings.");
        }
        else if (candidateGate.ErrorFindingCount < baselineGate.ErrorFindingCount)
        {
            improved = true;
            result.Notes.push_back("Candidate reduced error findings.");
        }

        if (candidateGate.WarningFindingCount > baselineGate.WarningFindingCount)
        {
            regressed = true;
            result.Notes.push_back("Candidate increased warning findings.");
        }
        else if (candidateGate.WarningFindingCount < baselineGate.WarningFindingCount)
        {
            improved = true;
            result.Notes.push_back("Candidate reduced warning findings.");
        }

        constexpr double tolerance = 1e-6;
        if (result.DurationDeltaSeconds < -tolerance)
        {
            improved = true;
            result.Notes.push_back("Candidate reduced total cycle duration.");
        }
        else if (result.DurationDeltaSeconds > tolerance)
        {
            result.Notes.push_back("Candidate increased total cycle duration.");
        }

        if (result.PathDeltaMeters < -tolerance)
        {
            improved = true;
            result.Notes.push_back("Candidate reduced TCP path length.");
        }
        else if (result.PathDeltaMeters > tolerance)
        {
            result.Notes.push_back("Candidate increased TCP path length.");
        }

        if (regressed)
        {
            result.Status = RegressionStatus::Regressed;
        }
        else if (improved)
        {
            result.Status = RegressionStatus::Improved;
        }
        return result;
    }

    std::string ProjectRegressionAnalyzer::BuildHtmlReport(
        const apex::io::StudioProject& baseline,
        const apex::io::StudioProject& candidate,
        const ProjectRegressionResult& result) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Project Regression</title>"
             << "<style>body{font-family:Arial,sans-serif;margin:24px;}table{border-collapse:collapse;width:100%;}"
             << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}th{background:#f5f5f5;}"
             << ".IMPROVED{color:#0a7d18;font-weight:bold;}.EQUIVALENT{color:#1b4d91;font-weight:bold;}.REGRESSED{color:#b00020;font-weight:bold;}"
             << "</style></head><body>";
        html << "<h1>ApexRoboticsStudio Regression Report</h1>";
        html << "<p><strong>Baseline:</strong> " << baseline.ProjectName << "<br><strong>Candidate:</strong> " << candidate.ProjectName << "</p>";
        html << "<h2>Overall Status: <span class=\"" << ToString(result.Status) << "\">" << ToString(result.Status) << "</span></h2>";
        html << "<table><thead><tr><th>Metric</th><th>Baseline</th><th>Candidate</th><th>Delta</th></tr></thead><tbody>";
        html << "<tr><td>Gate status</td><td>" << apex::quality::ToString(result.BaselineGateStatus) << "</td><td>" << apex::quality::ToString(result.CandidateGateStatus) << "</td><td>n/a</td></tr>";
        html << "<tr><td>Collision free</td><td>" << (result.BaselineCollisionFree ? "YES" : "NO") << "</td><td>" << (result.CandidateCollisionFree ? "YES" : "NO") << "</td><td>n/a</td></tr>";
        html << "<tr><td>Total duration (s)</td><td>" << result.BaselineDurationSeconds << "</td><td>" << result.CandidateDurationSeconds << "</td><td>" << result.DurationDeltaSeconds << "</td></tr>";
        html << "<tr><td>Total path (m)</td><td>" << result.BaselinePathLengthMeters << "</td><td>" << result.CandidatePathLengthMeters << "</td><td>" << result.PathDeltaMeters << "</td></tr>";
        html << "<tr><td>Error findings</td><td>" << result.BaselineErrorFindings << "</td><td>" << result.CandidateErrorFindings << "</td><td>" << (static_cast<long long>(result.CandidateErrorFindings) - static_cast<long long>(result.BaselineErrorFindings)) << "</td></tr>";
        html << "<tr><td>Warning findings</td><td>" << result.BaselineWarningFindings << "</td><td>" << result.CandidateWarningFindings << "</td><td>" << (static_cast<long long>(result.CandidateWarningFindings) - static_cast<long long>(result.BaselineWarningFindings)) << "</td></tr>";
        html << "</tbody></table>";
        html << "<h2>Notes</h2><ul>";
        for (const auto& note : result.Notes)
        {
            html << "<li>" << note << "</li>";
        }
        html << "</ul></body></html>";
        return html.str();
    }

    void ProjectRegressionAnalyzer::SaveHtmlReport(
        const apex::io::StudioProject& baseline,
        const apex::io::StudioProject& candidate,
        const ProjectRegressionResult& result,
        const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write project regression report: " + filePath.string());
        }
        stream << BuildHtmlReport(baseline, candidate, result);
    }
}
