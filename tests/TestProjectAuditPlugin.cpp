#include "TestFramework.h"
#include "apex/core/SerialRobotModel.h"
#include "apex/extension/ProjectAuditPlugin.h"
#include "apex/io/StudioProject.h"
#include "apex/workcell/CollisionWorld.h"

ARS_TEST(TestProjectAuditPluginRegistryProducesFindings)
{
    apex::io::StudioProject project;
    project.ProjectName = "Audit";
    project.RobotName = "Robot";
    project.Joints.push_back({"J1", 0.3, -1.0, 1.0});
    project.Motion.Start.JointAnglesRad = {0.99};
    project.Motion.Goal.JointAnglesRad = {-0.99};

    apex::core::SerialRobotModel robot("Robot");
    robot.AddRevoluteJoint(project.Joints.front());
    apex::workcell::CollisionWorld world;

    const apex::extension::ProjectAuditPluginRegistry registry;
    const auto findings = registry.RunBuiltInAudit(project, robot, world);

    apex::tests::Require(!findings.empty(), "Expected built-in audit plugins to produce findings");
}
