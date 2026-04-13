#include "TestFramework.h"

#include "apex/recipe/ProcessRecipe.h"

ARS_TEST(TestProcessRecipeApplicatorUpdatesQualityGateAndSegments)
{
    apex::io::StudioProject project;
    project.ProjectName = "RecipeBase";
    project.RobotName = "Robot";
    constexpr double limit = apex::core::Pi();
    project.Joints.push_back({"J1", 0.3, -limit, limit});
    project.Motion.Start.JointAnglesRad = {0.0};
    project.Motion.Goal.JointAnglesRad = {0.1};
    project.Job.Name = "DemoJob";
    project.Job.Waypoints.push_back({"A", {{0.0}}});
    project.Job.Waypoints.push_back({"B", {{0.1}}});
    project.Job.Segments.push_back({"Move", "A", "B", 6, 2.0, "baseline"});

    apex::recipe::ProcessRecipe recipe;
    recipe.Name = "Paint";
    recipe.ProcessFamily = "painting";
    recipe.DefaultSampleCount = 16;
    recipe.SegmentDurationScale = 1.5;
    recipe.ProcessTagPrefix = "paint";
    recipe.MaxPeakTcpSpeedMetersPerSecond = 1.2;

    const apex::recipe::ProcessRecipeApplicator applicator;
    const auto result = applicator.ApplyRecipe(project, recipe);

    apex::tests::Require(result.Metadata.ProcessFamily == "painting", "Recipe must update process family.");
    apex::tests::Require(result.Job.Segments[0].SampleCount == 16, "Recipe must update sample count.");
    apex::tests::RequireNear(result.Job.Segments[0].DurationSeconds, 3.0, 1e-9, "Recipe must scale segment duration.");
    apex::tests::Require(result.Job.Segments[0].ProcessTag == "paint.baseline", "Recipe must prefix process tag.");
    apex::tests::RequireNear(result.QualityGate.MaxPeakTcpSpeedMetersPerSecond, 1.2, 1e-9, "Recipe must update quality gate.");
}
