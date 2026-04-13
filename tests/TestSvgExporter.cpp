#include "TestFramework.h"
#include "apex/core/MathTypes.h"
#include "apex/core/SerialRobotModel.h"
#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/visualization/SvgExporter.h"
#include "apex/workcell/CollisionWorld.h"

ARS_TEST(TestSvgExporterProducesSvgDocument)
{
    apex::core::SerialRobotModel robot("SvgRobot");
    robot.AddRevoluteJoint({ "J1", 1.0, -apex::core::Pi(), apex::core::Pi() });
    apex::workcell::CollisionWorld world;
    world.AddObstacle({ "Box", {{0.6, -0.2, -0.1}, {0.8, 0.2, 0.1}} });

    const apex::planning::JointTrajectoryPlanner planner;
    const auto plan = planner.PlanLinearJointMotion(robot, {{0.0}}, {{0.5}}, 5, 1.0, world, 0.05);

    const apex::visualization::SvgExporter exporter;
    const std::string svg = exporter.BuildProjectTopViewSvg(robot, world, plan);

    apex::tests::Require(svg.find("<svg") != std::string::npos, "SVG output should contain <svg>");
    apex::tests::Require(svg.find("SvgRobot") != std::string::npos, "SVG output should include robot name");
}
