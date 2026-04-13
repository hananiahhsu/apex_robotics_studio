#include "TestFramework.h"

#include "apex/catalog/ProjectCatalog.h"
#include "apex/core/MathTypes.h"
#include "apex/io/StudioProject.h"

ARS_TEST(TestProjectCatalogBuildsItems)
{
    apex::io::StudioProject project;
    project.SchemaVersion = 2;
    project.ProjectName = "Catalog Demo";
    project.RobotName = "Robot";
    constexpr double limit = apex::core::Pi();
    project.Joints.push_back({"J1", 0.5, -limit, limit});
    project.Motion.Start.JointAnglesRad = {0.0};
    project.Motion.Goal.JointAnglesRad = {apex::core::DegreesToRadians(10.0)};
    project.Obstacles.push_back({"Fixture", {{0.0,0.0,0.0},{1.0,1.0,1.0}}});
    project.Job.Name = "Job";
    project.Job.Waypoints.push_back({"Home", {{0.0}}});
    project.Job.Waypoints.push_back({"Goal", {{apex::core::DegreesToRadians(10.0)}}});
    project.Job.Segments.push_back({"Move", "Home", "Goal", 12, 2.0, "dry_run"});

    const apex::catalog::ProjectCatalogBuilder builder;
    const auto catalog = builder.Build(project);
    apex::tests::Require(catalog.JointCount == 1, "Catalog should count joints.");
    apex::tests::Require(catalog.ObstacleCount == 1, "Catalog should count obstacles.");
    apex::tests::Require(catalog.WaypointCount == 2, "Catalog should count waypoints.");
    apex::tests::Require(!catalog.Items.empty(), "Catalog should contain items.");
    const auto textTree = builder.BuildTextTree(catalog);
    apex::tests::Require(textTree.find("Catalog Demo") != std::string::npos, "Text tree should mention project name.");
}
