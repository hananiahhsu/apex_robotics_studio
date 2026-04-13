#include "TestFramework.h"
#include "apex/analysis/TrajectoryAnalyzer.h"
#include "apex/core/MathTypes.h"
#include "apex/core/SerialRobotModel.h"
#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/workcell/CollisionWorld.h"

ARS_TEST(TestTrajectoryAnalyzerComputesPathMetrics)
{
    apex::core::SerialRobotModel robot("TestRobot");
    robot.AddRevoluteJoint({ "J1", 1.0, -apex::core::Pi(), apex::core::Pi() });
    robot.AddRevoluteJoint({ "J2", 0.5, -apex::core::Pi(), apex::core::Pi() });

    apex::workcell::CollisionWorld world;
    const apex::planning::JointTrajectoryPlanner planner;
    const apex::core::JointState start{ { 0.0, 0.0 } };
    const apex::core::JointState goal{ { 0.4, -0.2 } };
    const auto plan = planner.PlanLinearJointMotion(robot, start, goal, 10, 2.0, world, 0.05);

    const apex::analysis::TrajectoryAnalyzer analyzer;
    const auto report = analyzer.Analyze(robot, world, plan, 0.05, 0.3);

    apex::tests::Require(report.PathLengthMeters > 0.0, "Path length should be positive");
    apex::tests::Require(report.PeakTcpSpeedMetersPerSecond > 0.0, "Peak TCP speed should be positive");
    apex::tests::Require(report.JointVelocityMetrics.size() == 2, "Velocity metrics should be produced for each joint");
}
