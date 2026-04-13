#include "TestFramework.h"

#include "apex/governance/ProjectApproval.h"
#include "apex/integration/IntegrationExport.h"

#include <filesystem>

ARS_TEST(TestIntegrationExportWritesArtifacts)
{
    apex::io::StudioProject project;
    project.ProjectName = "Integration Test";
    project.RobotName = "Robot";
    project.Obstacles.push_back({"Fixture", {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}}});

    apex::simulation::JobSimulationResult simulation;
    simulation.JobName = "Job";
    simulation.CollisionFree = true;
    simulation.TotalDurationSeconds = 4.0;
    simulation.TotalPathLengthMeters = 1.5;
    apex::simulation::JobSegmentResult segment;
    segment.Segment.Name = "Move";
    segment.Segment.ProcessTag = "tag";
    segment.Quality.EstimatedCycleTimeSeconds = 4.0;
    segment.Quality.PathLengthMeters = 1.5;
    segment.Quality.CollisionFree = true;
    simulation.SegmentResults.push_back(segment);

    apex::quality::GateEvaluation gate;
    gate.Status = apex::quality::GateStatus::Pass;
    gate.CollisionFree = true;

    apex::governance::ProjectApprovalVerification approval;
    approval.OverallDecision = apex::governance::ApprovalDecision::Approved;

    const auto outputDir = std::filesystem::temp_directory_path() / "apex_integration_export_test";
    std::vector<apex::extension::PluginFinding> findings;
    findings.push_back({apex::extension::FindingSeverity::Warning, "warning"});

    const auto summary = apex::integration::IntegrationPackageExporter().Export(project, simulation, gate, findings, outputDir, &approval);

    apex::tests::Require(summary.ApprovalDecision == apex::governance::ApprovalDecision::Approved, "Approval decision should propagate to export summary.");
    apex::tests::Require(std::filesystem::exists(outputDir / "summary.json"), "Summary JSON should exist.");
    apex::tests::Require(std::filesystem::exists(outputDir / "segments.csv"), "Segments CSV should exist.");
    apex::tests::Require(std::filesystem::exists(outputDir / "findings.csv"), "Findings CSV should exist.");
    apex::tests::Require(std::filesystem::exists(outputDir / "obstacles.csv"), "Obstacles CSV should exist.");

    std::error_code ec;
    std::filesystem::remove_all(outputDir, ec);
}
