#include "TestFramework.h"
#include "apex/core/MathTypes.h"
#include "apex/core/SerialRobotModel.h"

ARS_TEST(TestForwardKinematicsForStraightConfiguration)
{
    apex::core::SerialRobotModel robot("TestRobot");
    robot.AddRevoluteJoint({ "J1", 1.0, -apex::core::Pi(), apex::core::Pi() });
    robot.AddRevoluteJoint({ "J2", 1.0, -apex::core::Pi(), apex::core::Pi() });

    const apex::core::JointState state{ { 0.0, 0.0 } };
    const auto tcp = robot.ComputeTcpPose(state);

    apex::tests::RequireNear(tcp.X, 2.0, 1e-9, "TCP X should be 2.0");
    apex::tests::RequireNear(tcp.Y, 0.0, 1e-9, "TCP Y should be 0.0");
}

ARS_TEST(TestForwardKinematicsForRightAngle)
{
    apex::core::SerialRobotModel robot("TestRobot");
    robot.AddRevoluteJoint({ "J1", 1.0, -apex::core::Pi(), apex::core::Pi() });
    robot.AddRevoluteJoint({ "J2", 1.0, -apex::core::Pi(), apex::core::Pi() });

    const apex::core::JointState state{ { apex::core::Pi() / 2.0, 0.0 } };
    const auto tcp = robot.ComputeTcpPose(state);

    apex::tests::RequireNear(tcp.X, 0.0, 1e-9, "TCP X should be near zero");
    apex::tests::RequireNear(tcp.Y, 2.0, 1e-9, "TCP Y should be 2.0");
}
