#include "TestFramework.h"

#include "apex/quality/ReleaseGate.h"

ARS_TEST(TestReleaseGateFailsOnErrorsAndCollisions)
{
    apex::io::StudioProject project;
    project.ProjectName = "Gate";
    project.QualityGate.RequireCollisionFree = true;
    project.QualityGate.MaxErrorFindings = 0;
    project.QualityGate.MaxWarningFindings = 1;
    project.QualityGate.MaxPeakTcpSpeedMetersPerSecond = 1.0;
    project.QualityGate.MaxAverageTcpSpeedMetersPerSecond = 1.0;

    apex::simulation::JobSimulationResult simulation;
    simulation.CollisionFree = false;
    apex::simulation::JobSegmentResult segment;
    segment.Quality.PeakTcpSpeedMetersPerSecond = 1.5;
    segment.Quality.AverageTcpSpeedMetersPerSecond = 0.8;
    simulation.SegmentResults.push_back(segment);

    std::vector<apex::extension::PluginFinding> findings;
    findings.push_back({apex::extension::FindingSeverity::Error, "Critical fault"});

    const apex::quality::ReleaseGateEvaluator evaluator;
    const auto evaluation = evaluator.Evaluate(project, simulation, findings);

    apex::tests::Require(evaluation.Status == apex::quality::GateStatus::Fail, "Gate must fail.");
}
