#include "TestFramework.h"
#include "apex/core/MathTypes.h"
#include "apex/core/SerialRobotModel.h"
#include "apex/workcell/CollisionWorld.h"

ARS_TEST(TestCollisionDetectedAgainstObstacle)
{
    apex::core::SerialRobotModel robot("TestRobot");
    robot.AddRevoluteJoint({ "J1", 1.0, -apex::core::Pi(), apex::core::Pi() });

    apex::workcell::CollisionWorld world;
    world.AddObstacle({ "Box", {{0.9, -0.2, -0.2}, {1.1, 0.2, 0.2}} });

    const apex::core::JointState state{ { 0.0 } };
    const auto report = world.EvaluateRobotState(robot, state, 0.15);

    apex::tests::Require(report.HasCollision, "Collision should have been detected");
    apex::tests::Require(report.Events.size() == 1, "Exactly one collision event is expected");
}
