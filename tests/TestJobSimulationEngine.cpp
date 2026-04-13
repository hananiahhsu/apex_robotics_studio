#include "TestFramework.h"
#include "apex/core/MathTypes.h"
#include "apex/core/SerialRobotModel.h"
#include "apex/simulation/JobSimulationEngine.h"
#include "apex/workcell/CollisionWorld.h"

ARS_TEST(TestJobSimulationEngineRunsSegments)
{
    apex::core::SerialRobotModel robot("TestRobot");
    robot.AddRevoluteJoint({"J1", 0.4, -apex::core::Pi(), apex::core::Pi()});
    robot.AddRevoluteJoint({"J2", 0.3, -apex::core::Pi(), apex::core::Pi()});

    apex::job::RobotJob job;
    job.Name = "PickAndPlace";
    job.LinkSafetyRadius = 0.05;
    job.Waypoints.push_back({"Home", {{0.0, 0.0}}});
    job.Waypoints.push_back({"Approach", {{0.2, -0.1}}});
    job.Waypoints.push_back({"Place", {{0.4, 0.1}}});
    job.Segments.push_back({"MoveA", "Home", "Approach", 8, 1.5, "approach"});
    job.Segments.push_back({"MoveB", "Approach", "Place", 8, 1.5, "place"});

    apex::workcell::CollisionWorld world;
    const apex::simulation::JobSimulationEngine engine;
    const auto result = engine.Simulate(robot, world, job);

    apex::tests::Require(result.SegmentResults.size() == 2, "Expected two segment results");
    apex::tests::Require(result.TotalSamples >= 16, "Expected total samples to be accumulated");
    apex::tests::RequireNear(result.TotalDurationSeconds, 3.0, 1e-9, "Expected total duration to match segment durations");
}
