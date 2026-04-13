#include "TestFramework.h"

#include "apex/regression/ProjectRegression.h"

ARS_TEST(TestProjectRegressionDetectsRegression)
{
    apex::simulation::JobSimulationResult baselineSimulation;
    baselineSimulation.TotalDurationSeconds = 5.0;
    baselineSimulation.TotalPathLengthMeters = 10.0;
    baselineSimulation.CollisionFree = true;

    apex::simulation::JobSimulationResult candidateSimulation = baselineSimulation;
    candidateSimulation.TotalDurationSeconds = 6.0;
    candidateSimulation.CollisionFree = false;

    apex::quality::GateEvaluation baselineGate;
    baselineGate.Status = apex::quality::GateStatus::Pass;
    baselineGate.CollisionFree = true;

    apex::quality::GateEvaluation candidateGate;
    candidateGate.Status = apex::quality::GateStatus::Fail;
    candidateGate.CollisionFree = false;
    candidateGate.ErrorFindingCount = 1;

    const apex::regression::ProjectRegressionAnalyzer analyzer;
    const auto result = analyzer.Evaluate(baselineSimulation, baselineGate, candidateSimulation, candidateGate);

    apex::tests::Require(result.Status == apex::regression::RegressionStatus::Regressed, "Worse gate and collisions should regress.");
}

ARS_TEST(TestProjectRegressionDetectsImprovement)
{
    apex::simulation::JobSimulationResult baselineSimulation;
    baselineSimulation.TotalDurationSeconds = 8.0;
    baselineSimulation.TotalPathLengthMeters = 12.0;
    baselineSimulation.CollisionFree = false;

    apex::simulation::JobSimulationResult candidateSimulation;
    candidateSimulation.TotalDurationSeconds = 7.0;
    candidateSimulation.TotalPathLengthMeters = 10.0;
    candidateSimulation.CollisionFree = true;

    apex::quality::GateEvaluation baselineGate;
    baselineGate.Status = apex::quality::GateStatus::Hold;
    baselineGate.CollisionFree = false;
    baselineGate.WarningFindingCount = 2;

    apex::quality::GateEvaluation candidateGate;
    candidateGate.Status = apex::quality::GateStatus::Pass;
    candidateGate.CollisionFree = true;
    candidateGate.WarningFindingCount = 1;

    const apex::regression::ProjectRegressionAnalyzer analyzer;
    const auto result = analyzer.Evaluate(baselineSimulation, baselineGate, candidateSimulation, candidateGate);

    apex::tests::Require(result.Status == apex::regression::RegressionStatus::Improved, "Better gate and cycle metrics should improve.");
}
