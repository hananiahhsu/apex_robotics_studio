#include "TestFramework.h"

#include "apex/catalog/ProjectCatalog.h"
#include "apex/core/MathTypes.h"
#include "apex/dashboard/WorkbenchDashboard.h"
#include "apex/io/StudioProject.h"
#include "apex/session/ExecutionSession.h"
#include "apex/simulation/JobSimulationEngine.h"

ARS_TEST(TestWorkbenchDashboardBuildsHtml)
{
    apex::io::StudioProject project;
    project.SchemaVersion = 2;
    project.ProjectName = "Dashboard Demo";
    project.RobotName = "Robot";
    constexpr double limit = apex::core::Pi();
    project.Joints.push_back({"J1", 0.5, -limit, limit});
    project.Motion.Start.JointAnglesRad = {0.0};
    project.Motion.Goal.JointAnglesRad = {0.1};
    project.Job.Name = "Job";
    project.Job.Waypoints.push_back({"Home", {{0.0}}});
    project.Job.Waypoints.push_back({"Goal", {{0.1}}});
    project.Job.Segments.push_back({"Move", "Home", "Goal", 4, 1.0, "dry_run"});

    apex::catalog::ProjectCatalogBuilder catalogBuilder;
    const auto catalog = catalogBuilder.Build(project);

    apex::simulation::JobSimulationResult sim;
    sim.JobName = "Job";
    sim.CollisionFree = true;
    sim.TotalDurationSeconds = 1.0;
    sim.TotalPathLengthMeters = 0.25;
    sim.SegmentResults.push_back({project.Job.Segments.front(), {}, {true, 0, 0.25, 0.1, 1.0, 0.25, 0.3, static_cast<std::size_t>(-1), {}, {}}});

    apex::session::SessionRecorder recorder;
    recorder.SetTitle("Dashboard Session");
    recorder.SetProjectName(project.ProjectName);
    recorder.SetJobName(project.Job.Name);
    recorder.AddEvent("dashboard", "info", "Generated dashboard.");

    apex::dashboard::WorkbenchDashboardGenerator generator;
    const auto html = generator.BuildProjectDashboard(project, catalog, sim, {}, recorder.GetSummary(), "<svg></svg>");
    apex::tests::Require(html.find("Workbench") != std::string::npos, "Dashboard HTML should contain workstation title.");
    apex::tests::Require(html.find("Dashboard Demo") != std::string::npos, "Dashboard HTML should contain project name.");
}
