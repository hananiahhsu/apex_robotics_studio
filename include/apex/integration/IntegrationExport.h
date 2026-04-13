#pragma once

#include "apex/governance/ProjectApproval.h"
#include "apex/io/StudioProject.h"
#include "apex/extension/ProjectAuditPlugin.h"
#include "apex/quality/ReleaseGate.h"
#include "apex/simulation/JobSimulationEngine.h"

#include <filesystem>
#include <string>

namespace apex::integration
{
    struct IntegrationExportSummary
    {
        std::string ProjectName;
        std::string ProjectFingerprint;
        std::string ExportedAtUtc;
        apex::quality::GateStatus GateStatus = apex::quality::GateStatus::Pass;
        apex::governance::ApprovalDecision ApprovalDecision = apex::governance::ApprovalDecision::Pending;
        bool CollisionFree = true;
        std::size_t SegmentCount = 0;
        std::size_t ObstacleCount = 0;
        std::size_t FindingCount = 0;
        double TotalDurationSeconds = 0.0;
        double TotalPathLengthMeters = 0.0;
    };

    class IntegrationPackageExporter
    {
    public:
        [[nodiscard]] IntegrationExportSummary Export(
            const apex::io::StudioProject& project,
            const apex::simulation::JobSimulationResult& simulation,
            const apex::quality::GateEvaluation& gate,
            const std::vector<apex::extension::PluginFinding>& findings,
            const std::filesystem::path& outputDirectory,
            const apex::governance::ProjectApprovalVerification* approvalVerification = nullptr) const;
    };
}
