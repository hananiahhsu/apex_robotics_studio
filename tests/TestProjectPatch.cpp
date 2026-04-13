#include "TestFramework.h"

#include "apex/edit/ProjectPatch.h"
#include "apex/io/StudioProjectSerializer.h"

#include <filesystem>

ARS_TEST(TestProjectPatchApplysObstacleTranslationAndMotionOverride)
{
    apex::io::StudioProject project;
    project.ProjectName = "PatchBase";
    project.RobotName = "Robot";
    constexpr double limit = apex::core::Pi();
    project.Joints.push_back({"J1", 0.3, -limit, limit});
    project.Motion.Start.JointAnglesRad = {0.0};
    project.Motion.Goal.JointAnglesRad = {0.1};
    project.Motion.SampleCount = 10;
    project.Motion.DurationSeconds = 2.0;
    project.Obstacles.push_back({"Fixture", {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}}});

    apex::edit::ProjectPatch patch;
    patch.Name = "MoveFixture";
    patch.Motion.HasSampleCount = true;
    patch.Motion.SampleCount = 20;
    patch.ObstaclesToTranslate.push_back({"Fixture", {1.0, 0.0, 0.0}});
    patch.ObstaclesToAdd.push_back({"Guard", {2.0, 2.0, 0.0}, {3.0, 3.0, 1.0}});

    const apex::edit::ProjectEditor editor;
    const auto result = editor.ApplyPatch(project, patch);

    apex::tests::Require(result.Motion.SampleCount == 20, "Patch must override motion sample count.");
    apex::tests::Require(result.Obstacles.size() == 2, "Patch must add a new obstacle.");
    apex::tests::RequireNear(result.Obstacles[0].Bounds.Min.X, 1.0, 1e-9, "Translated obstacle min.x mismatch.");
}
