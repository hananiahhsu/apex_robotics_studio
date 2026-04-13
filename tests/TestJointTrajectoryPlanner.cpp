#include "TestFramework.h"
#include "apex/core/MathTypes.h"
#include "apex/core/SerialRobotModel.h"
#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/workcell/CollisionWorld.h"

ARS_TEST(TestPlannerProducesExpectedSampleCount)
{
    apex::core::SerialRobotModel robot("TestRobot");
    robot.AddRevoluteJoint({ "J1", 1.0, -apex::core::Pi(), apex::core::Pi() });
    robot.AddRevoluteJoint({ "J2", 1.0, -apex::core::Pi(), apex::core::Pi() });

    const apex::workcell::CollisionWorld world;
    const apex::planning::JointTrajectoryPlanner planner;

    const apex::core::JointState start{ { 0.0, 0.0 } };
    const apex::core::JointState goal{ { 0.5, 0.25 } };
    const auto plan = planner.PlanLinearJointMotion(robot, start, goal, 8, 2.0, world, 0.05);

    apex::tests::Require(plan.Samples.size() == 8, "Planner should generate 8 samples");
    apex::tests::RequireNear(plan.Samples.back().TimeSeconds, 2.0, 1e-9, "Last sample time should match duration");
}
