#include "TestFramework.h"

#include "apex/template/ProjectTemplate.h"
#include "apex/io/StudioProjectSerializer.h"

#include <filesystem>

ARS_TEST(TestProjectTemplateInstantiate)
{
    apex::templating::ProjectTemplate projectTemplate;
    projectTemplate.Name = "Test Template";
    projectTemplate.DefaultProjectName = "Instantiated";
    projectTemplate.DefaultCellName = "Cell-T1";
    projectTemplate.DefaultProcessFamily = "welding";
    projectTemplate.DefaultOwner = "integration";
    projectTemplate.DefaultNotesSuffix = "Template-applied.";
    projectTemplate.SampleCountOverride = 24;
    projectTemplate.DurationScale = 1.25;
    projectTemplate.LinkSafetyRadius = 0.11;
    projectTemplate.ObstacleOffset = {0.1, 0.0, 0.0};

    apex::io::StudioProject base;
    base.ProjectName = "Base";
    base.RobotName = "Robot";
    constexpr double limit = 3.14;
    base.Joints.push_back({"J1", 0.4, -limit, limit});
    base.Motion.Start.JointAnglesRad = {0.0};
    base.Motion.Goal.JointAnglesRad = {1.0};
    base.Motion.SampleCount = 10;
    base.Motion.DurationSeconds = 2.0;
    base.Motion.LinkSafetyRadius = 0.08;
    base.Obstacles.push_back({"Fixture", {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}}});
    base.Job.Name = "Job";
    base.Job.LinkSafetyRadius = 0.08;
    base.Job.Waypoints.push_back({"A", {{0.0}}});
    base.Job.Waypoints.push_back({"B", {{1.0}}});
    base.Job.Segments.push_back({"Move", "A", "B", 10, 2.0, "process"});
    projectTemplate.BaseProject = base;

    apex::templating::ProjectTemplateInstantiator instantiator;
    apex::templating::TemplateOverrides overrides;
    overrides.ProjectName = "Candidate";
    const auto project = instantiator.Instantiate(projectTemplate, overrides);

    apex::tests::Require(project.ProjectName == "Candidate", "Project name override should win.");
    apex::tests::Require(project.Metadata.CellName == "Cell-T1", "Default cell name should be applied.");
    apex::tests::Require(project.Motion.SampleCount == 24, "Motion sample count should be overridden by template.");
    apex::tests::RequireNear(project.Motion.DurationSeconds, 2.5, 1e-9, "Duration scale should be applied.");
    apex::tests::RequireNear(project.Job.LinkSafetyRadius, 0.11, 1e-9, "Job safety radius should be overridden.");
    apex::tests::RequireNear(project.Obstacles.front().Bounds.Min.X, 0.1, 1e-9, "Obstacle offset should be applied.");
}

ARS_TEST(TestProjectTemplateRoundTrip)
{
    const std::filesystem::path filePath = std::filesystem::temp_directory_path() / "apex_project_template_test.arstemplate";

    apex::templating::ProjectTemplate projectTemplate;
    projectTemplate.Name = "RoundTrip";
    projectTemplate.Description = "Template serializer test";
    projectTemplate.DefaultOwner = "owner";
    projectTemplate.BaseProject.ProjectName = "Base";
    projectTemplate.BaseProject.RobotName = "Robot";
    constexpr double limit = 3.14;
    projectTemplate.BaseProject.Joints.push_back({"J1", 0.5, -limit, limit});
    projectTemplate.BaseProject.Motion.Start.JointAnglesRad = {0.0};
    projectTemplate.BaseProject.Motion.Goal.JointAnglesRad = {0.5};

    apex::templating::ProjectTemplateLoader loader;
    loader.SaveToFile(projectTemplate, filePath);
    const auto loaded = loader.LoadFromFile(filePath);

    apex::tests::Require(loaded.Name == projectTemplate.Name, "Template name should survive round trip.");
    apex::tests::Require(loaded.BaseProject.RobotName == "Robot", "Embedded base project should round trip.");

    std::error_code ec;
    std::filesystem::remove(filePath, ec);
}
