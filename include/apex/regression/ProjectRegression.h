#pragma once

#include "apex/io/StudioProject.h"
#include "apex/quality/ReleaseGate.h"
#include "apex/simulation/JobSimulationEngine.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::regression
{
    enum class RegressionStatus
    {
        Improved,
        Equivalent,
        Regressed
    };

    struct ProjectRegressionResult
    {
        RegressionStatus Status = RegressionStatus::Equivalent;
        apex::quality::GateStatus BaselineGateStatus = apex::quality::GateStatus::Pass;
        apex::quality::GateStatus CandidateGateStatus = apex::quality::GateStatus::Pass;
        double BaselineDurationSeconds = 0.0;
        double CandidateDurationSeconds = 0.0;
        double DurationDeltaSeconds = 0.0;
        double BaselinePathLengthMeters = 0.0;
        double CandidatePathLengthMeters = 0.0;
        double PathDeltaMeters = 0.0;
        std::size_t BaselineErrorFindings = 0;
        std::size_t CandidateErrorFindings = 0;
        std::size_t BaselineWarningFindings = 0;
        std::size_t CandidateWarningFindings = 0;
        bool BaselineCollisionFree = true;
        bool CandidateCollisionFree = true;
        std::vector<std::string> Notes;
    };

    class ProjectRegressionAnalyzer
    {
    public:
        [[nodiscard]] ProjectRegressionResult Evaluate(
            const apex::simulation::JobSimulationResult& baselineSimulation,
            const apex::quality::GateEvaluation& baselineGate,
            const apex::simulation::JobSimulationResult& candidateSimulation,
            const apex::quality::GateEvaluation& candidateGate) const;

        [[nodiscard]] std::string BuildHtmlReport(
            const apex::io::StudioProject& baseline,
            const apex::io::StudioProject& candidate,
            const ProjectRegressionResult& result) const;

        void SaveHtmlReport(
            const apex::io::StudioProject& baseline,
            const apex::io::StudioProject& candidate,
            const ProjectRegressionResult& result,
            const std::filesystem::path& filePath) const;
    };

    [[nodiscard]] std::string ToString(RegressionStatus status) noexcept;
}
