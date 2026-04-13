#include "apex/analysis/TrajectoryAnalyzer.h"
#include "apex/bundle/ProjectBundle.h"
#include "apex/catalog/ProjectCatalog.h"
#include "apex/diff/ProjectDiff.h"
#include "apex/edit/ProjectPatch.h"
#include "apex/dashboard/WorkbenchDashboard.h"
#include "apex/governance/ProjectApproval.h"
#include "apex/integration/IntegrationExport.h"
#include "apex/integration/Ros2WorkspaceExport.h"
#include "apex/delivery/DeliveryDossier.h"
#include "apex/core/MathTypes.h"
#include "apex/core/SerialRobotModel.h"
#include "apex/extension/ExternalAuditPlugin.h"
#include "apex/extension/ProjectAuditPlugin.h"
#include "apex/importer/UrdfImporter.h"
#include "apex/importer/RobotDescriptionInspector.h"
#include "apex/io/StudioProjectSerializer.h"
#include "apex/ops/BatchRunner.h"
#include "apex/session/ExecutionSession.h"
#include "apex/platform/RuntimeConfig.h"
#include "apex/revision/ProjectSnapshot.h"
#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/report/ProjectReportGenerator.h"
#include "apex/template/ProjectTemplate.h"
#include "apex/trace/AuditTrail.h"
#include "apex/workflow/AutomationFlow.h"
#include "apex/sweep/ParameterSweep.h"
#include "apex/recipe/ProcessRecipe.h"
#include "apex/quality/ReleaseGate.h"
#include "apex/regression/ProjectRegression.h"
#include "apex/simulation/JobSimulationEngine.h"
#include "apex/visualization/SvgExporter.h"
#include "apex/workcell/CollisionWorld.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace
{
    apex::core::SerialRobotModel BuildRobotFromProject(const apex::io::StudioProject& project)
    {
        apex::core::SerialRobotModel robot(project.RobotName);
        for (const auto& joint : project.Joints)
        {
            robot.AddRevoluteJoint(joint);
        }
        return robot;
    }

    apex::workcell::CollisionWorld BuildWorldFromProject(const apex::io::StudioProject& project)
    {
        apex::workcell::CollisionWorld world;
        for (const auto& obstacle : project.Obstacles)
        {
            world.AddObstacle(obstacle);
        }
        return world;
    }

    apex::io::StudioProject BuildDemoProject()
    {
        apex::io::StudioProject project;
        project.SchemaVersion = 2;
        project.ProjectName = "Demo Workcell Interview Scenario";
        project.RobotName = "Apex Painter 6A";
        constexpr double jointLimit = apex::core::Pi();
        project.Joints.push_back({ "J1", 0.45, -jointLimit, jointLimit });
        project.Joints.push_back({ "J2", 0.35, -jointLimit, jointLimit });
        project.Joints.push_back({ "J3", 0.30, -jointLimit, jointLimit });
        project.Joints.push_back({ "J4", 0.20, -jointLimit, jointLimit });
        project.Joints.push_back({ "J5", 0.15, -jointLimit, jointLimit });
        project.Joints.push_back({ "J6", 0.10, -jointLimit, jointLimit });
        project.Obstacles.push_back({ "Fixture_A", {{0.55, 0.10, -0.20}, {0.85, 0.40, 0.20}} });
        project.Obstacles.push_back({ "Safety_Fence", {{1.25, -0.90, -0.50}, {1.40, 0.90, 0.50}} });
        project.Obstacles.push_back({ "Table", {{0.25, -0.70, -0.20}, {0.85, -0.45, 0.15}} });
        project.Obstacles.push_back({ "PaintBooth", {{0.90, 0.55, -0.10}, {1.25, 0.95, 0.75}} });
        project.Metadata.CellName = "Cell-07-Paint";
        project.Metadata.ProcessFamily = "painting";
        project.Metadata.Owner = "hananiah";
        project.Metadata.Notes = "Interview-grade robot workstation scenario.";
        project.QualityGate.RequireCollisionFree = true;
        project.QualityGate.MaxErrorFindings = 0;
        project.QualityGate.MaxWarningFindings = 2;
        project.QualityGate.MaxPeakTcpSpeedMetersPerSecond = 1.8;
        project.QualityGate.MaxAverageTcpSpeedMetersPerSecond = 1.2;
        project.QualityGate.PreferredJointVelocityLimitRadPerSec = 1.5;

        project.Motion.Start.JointAnglesRad = {
            apex::core::DegreesToRadians(0.0), apex::core::DegreesToRadians(10.0), apex::core::DegreesToRadians(20.0),
            apex::core::DegreesToRadians(-15.0), apex::core::DegreesToRadians(5.0), apex::core::DegreesToRadians(0.0) };
        project.Motion.Goal.JointAnglesRad = {
            apex::core::DegreesToRadians(35.0), apex::core::DegreesToRadians(25.0), apex::core::DegreesToRadians(-30.0),
            apex::core::DegreesToRadians(15.0), apex::core::DegreesToRadians(-20.0), apex::core::DegreesToRadians(10.0) };
        project.Motion.SampleCount = 16;
        project.Motion.DurationSeconds = 4.0;
        project.Motion.LinkSafetyRadius = 0.08;

        project.Job.Name = "Door Panel Paint Cycle";
        project.Job.LinkSafetyRadius = 0.08;
        project.Job.Waypoints.push_back({"Home", {{
            apex::core::DegreesToRadians(0.0), apex::core::DegreesToRadians(15.0), apex::core::DegreesToRadians(20.0),
            apex::core::DegreesToRadians(-15.0), apex::core::DegreesToRadians(0.0), apex::core::DegreesToRadians(0.0)}}});
        project.Job.Waypoints.push_back({"Approach", {{
            apex::core::DegreesToRadians(18.0), apex::core::DegreesToRadians(22.0), apex::core::DegreesToRadians(5.0),
            apex::core::DegreesToRadians(-10.0), apex::core::DegreesToRadians(-8.0), apex::core::DegreesToRadians(4.0)}}});
        project.Job.Waypoints.push_back({"SprayStart", {{
            apex::core::DegreesToRadians(28.0), apex::core::DegreesToRadians(18.0), apex::core::DegreesToRadians(-15.0),
            apex::core::DegreesToRadians(10.0), apex::core::DegreesToRadians(-14.0), apex::core::DegreesToRadians(9.0)}}});
        project.Job.Waypoints.push_back({"SprayEnd", {{
            apex::core::DegreesToRadians(36.0), apex::core::DegreesToRadians(12.0), apex::core::DegreesToRadians(-22.0),
            apex::core::DegreesToRadians(16.0), apex::core::DegreesToRadians(-18.0), apex::core::DegreesToRadians(12.0)}}});
        project.Job.Waypoints.push_back({"Retreat", {{
            apex::core::DegreesToRadians(14.0), apex::core::DegreesToRadians(10.0), apex::core::DegreesToRadians(14.0),
            apex::core::DegreesToRadians(-6.0), apex::core::DegreesToRadians(6.0), apex::core::DegreesToRadians(2.0)}}});
        project.Job.Segments.push_back({"MoveToApproach", "Home", "Approach", 12, 2.0, "approach"});
        project.Job.Segments.push_back({"PaintStrokeA", "Approach", "SprayStart", 18, 2.5, "spray"});
        project.Job.Segments.push_back({"PaintStrokeB", "SprayStart", "SprayEnd", 18, 2.5, "spray"});
        project.Job.Segments.push_back({"MoveToHome", "SprayEnd", "Retreat", 12, 2.0, "retreat"});
        return project;
    }

    apex::planning::TrajectoryPlan PlanTrajectory(
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world,
        const apex::io::MotionRequest& motion)
    {
        const apex::planning::JointTrajectoryPlanner planner;
        return planner.PlanLinearJointMotion(robot, motion.Start, motion.Goal, motion.SampleCount, motion.DurationSeconds, world, motion.LinkSafetyRadius);
    }

    std::filesystem::path GetExecutablePath(const char* argv0)
    {
        return std::filesystem::absolute(std::filesystem::path(argv0));
    }

    std::filesystem::path GetSampleAuditToolPath(const std::filesystem::path& executableDirectory)
    {
#ifdef _WIN32
        return executableDirectory / "apex_audit_tool_sample.exe";
#else
        return executableDirectory / "apex_audit_tool_sample";
#endif
    }

    void PrintBanner()
    {
        std::cout << "ApexRoboticsStudio 1.7.0\n"
                  << "Robotics workstation interview project\n\n";
    }

    void PrintUsage()
    {
        std::cout << "Usage:\n"
                  << "  apex_robotics_studio demo\n"
                  << "  apex_robotics_studio summary\n"
                  << "  apex_robotics_studio export-demo-project [output.arsproject]\n"
                  << "  apex_robotics_studio export-runtime-config [output.ini]\n"
                  << "  apex_robotics_studio export-demo-batch [output.arsbatch]\n"
                  << "  apex_robotics_studio run-project <project.arsproject>\n"
                  << "  apex_robotics_studio run-job-project <project.arsproject> [runtime.ini] [plugin_dir]\n"
                  << "  apex_robotics_studio inspect-project <project.arsproject> [plugin_dir]\n"
                  << "  apex_robotics_studio list-plugins [plugin_dir]\n"
                  << "  apex_robotics_studio import-urdf <robot.{urdf|xacro}> [output.arsproject]\n"
                  << "  apex_robotics_studio import-robot-description <robot.{urdf|xacro}> [output.arsproject]\n"
                  << "  apex_robotics_studio render-project-svg <project.arsproject> [output.svg]\n"
                  << "  apex_robotics_studio report-project <project.arsproject> [output.html]\n"
                  << "  apex_robotics_studio report-job-project <project.arsproject> [output.html] [runtime.ini] [plugin_dir]\n"
                  << "  apex_robotics_studio bundle-project <project.arsproject> [runtime.ini] [output_dir] [plugin_dir]\n"
                  << "  apex_robotics_studio run-bundle <bundle_dir>\n"
                  << "  apex_robotics_studio run-batch <batch.arsbatch>\n"
                  << "  apex_robotics_studio report-batch <batch.arsbatch> [output.html]\n"
                  << "  apex_robotics_studio project-tree <project.arsproject>\n"
                  << "  apex_robotics_studio record-job-session <project.arsproject> [output_dir] [runtime.ini] [plugin_dir]\n"
                  << "  apex_robotics_studio dashboard-project <project.arsproject> [output.html] [runtime.ini] [plugin_dir]\n"
                  << "  apex_robotics_studio dashboard-batch <batch.arsbatch> [output.html]\n"
                  << "  apex_robotics_studio export-demo-recipe [output.arsrecipe]\n"
                  << "  apex_robotics_studio export-demo-patch [output.arspatch]\n"
                  << "  apex_robotics_studio apply-recipe <project.arsproject> <recipe.arsrecipe> [output.arsproject]\n"
                  << "  apex_robotics_studio apply-project-patch <project.arsproject> <patch.arspatch> [output.arsproject]\n"
                  << "  apex_robotics_studio gate-project <project.arsproject> [output.html] [runtime.ini] [plugin_dir]\n"
                  << "  apex_robotics_studio diff-project <lhs.arsproject> <rhs.arsproject> [output.txt]\n"
                  << "  apex_robotics_studio export-demo-template [output.arstemplate]\n"
                  << "  apex_robotics_studio instantiate-template <template.arstemplate> [output.arsproject] [project_name]\n"
                  << "  apex_robotics_studio regress-project <baseline.arsproject> <candidate.arsproject> [output.html] [runtime.ini] [plugin_dir]\n"
                  << "  apex_robotics_studio create-delivery-dossier <project.arsproject> [output_dir] [runtime.ini] [plugin_dir]\n"
                  << "  apex_robotics_studio export-demo-flow [output.arsflow]\n"
                  << "  apex_robotics_studio run-flow <flow.arsflow>\n"
                  << "  apex_robotics_studio export-demo-sweep [output.arssweep]\n"
                  << "  apex_robotics_studio run-sweep <sweep.arssweep>\n"
                  << "  apex_robotics_studio audit-project <project.arsproject> [output_dir] [runtime.ini] [plugin_dir] [approval.arsapproval]\n"
                  << "  apex_robotics_studio export-ros2-workspace <project.arsproject> [output_dir]\n";
    }

    void PrintRobotSummary(const apex::core::SerialRobotModel& robot)
    {
        std::cout << "Robot: " << robot.GetName() << "\n";
        std::cout << "Joint count: " << robot.GetJointCount() << "\n";
        for (std::size_t index = 0; index < robot.GetJoints().size(); ++index)
        {
            const auto& joint = robot.GetJoints()[index];
            std::cout << "  [" << index << "] " << joint.Name
                      << " link=" << joint.LinkLength << " m"
                      << " range=[" << apex::core::RadiansToDegrees(joint.MinAngleRad)
                      << ", " << apex::core::RadiansToDegrees(joint.MaxAngleRad) << "] deg\n";
        }
    }

    void WriteTrajectoryCsv(const apex::planning::TrajectoryPlan& plan, const std::filesystem::path& outputPath)
    {
        std::ofstream stream(outputPath);
        stream << "time_seconds,tcp_x,tcp_y,tcp_z,tcp_heading_deg,collision\n";
        for (const auto& sample : plan.Samples)
        {
            stream << std::fixed << std::setprecision(6)
                   << sample.TimeSeconds << ',' << sample.Tcp.X << ',' << sample.Tcp.Y << ',' << sample.Tcp.Z << ','
                   << apex::core::RadiansToDegrees(sample.Tcp.HeadingRad) << ',' << (sample.HasCollision ? 1 : 0) << '\n';
        }
        std::cout << "\nTCP path exported: " << outputPath.string() << "\n";
    }

    apex::platform::RuntimeConfig LoadRuntimeConfigOrDefault(const std::filesystem::path* filePath)
    {
        apex::platform::RuntimeConfig config;
        if (filePath != nullptr)
        {
            const apex::platform::RuntimeConfigLoader loader;
            config = loader.LoadFromFile(*filePath);
        }
        return config;
    }

    std::filesystem::path ResolveRelativeTo(const std::filesystem::path& anchorFile, const std::filesystem::path& candidate)
    {
        if (candidate.empty() || candidate.is_absolute())
        {
            return candidate;
        }
        return anchorFile.parent_path() / candidate;
    }

    std::filesystem::path EnsureDirectoryOrCurrent(const std::filesystem::path& path)
    {
        if (path.empty())
        {
            return std::filesystem::current_path();
        }
        return path;
    }

    std::filesystem::path PreparePluginWorkspace(const std::filesystem::path& outputDirectory, const std::filesystem::path& sampleAuditToolPath)
    {
        std::filesystem::create_directories(outputDirectory);
        if (!std::filesystem::exists(sampleAuditToolPath))
        {
            throw std::runtime_error("Sample audit tool not found: " + sampleAuditToolPath.string());
        }

        const auto copiedToolPath = outputDirectory / sampleAuditToolPath.filename();
        std::filesystem::copy_file(sampleAuditToolPath, copiedToolPath, std::filesystem::copy_options::overwrite_existing);
        std::ofstream stream(outputDirectory / "sample_safety_audit.arsplugin");
        if (!stream)
        {
            throw std::runtime_error("Failed to create plugin manifest in: " + outputDirectory.string());
        }
        stream << "; Generated plugin workspace\n[plugin]\nname=SampleSafetyAudit\n"
               << "executable=" << copiedToolPath.filename().generic_string() << "\n"
               << "arguments=audit {project} {output}\n";
        return outputDirectory;
    }

    int ExecutePlan(const apex::io::StudioProject& project)
    {
        const auto robot = BuildRobotFromProject(project);
        const auto world = BuildWorldFromProject(project);
        PrintRobotSummary(robot);
        std::cout << "\nWorkcell obstacles: " << world.GetObstacles().size() << "\n";
        for (const auto& obstacle : world.GetObstacles())
        {
            std::cout << "  - " << obstacle.Name
                      << " min=" << apex::core::ToString(obstacle.Bounds.Min)
                      << " max=" << apex::core::ToString(obstacle.Bounds.Max) << '\n';
        }

        const auto plan = PlanTrajectory(robot, world, project.Motion);
        const apex::analysis::TrajectoryAnalyzer analyzer;
        const auto quality = analyzer.Analyze(robot, world, plan, project.Motion.LinkSafetyRadius);

        std::cout << "\nTrajectory samples: " << plan.Samples.size() << "\n";
        std::cout << "Collision-free: " << (quality.CollisionFree ? "YES" : "NO") << "\n";
        std::cout << "Path length: " << quality.PathLengthMeters << " m\n";
        std::cout << "Peak TCP speed: " << quality.PeakTcpSpeedMetersPerSecond << " m/s\n\n";

        WriteTrajectoryCsv(plan, std::filesystem::current_path() / "apex_tcp_path.csv");
        return 0;
    }

    int RunProject(const std::filesystem::path& filePath)
    {
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        std::cout << "Loaded project: " << project.ProjectName << "\n";
        std::cout << "Project file: " << filePath.string() << "\n\n";
        return ExecutePlan(project);
    }

    std::vector<apex::extension::PluginFinding> GatherPluginFindings(
        const apex::io::StudioProject& project,
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world,
        const std::filesystem::path& projectFilePath,
        bool enableBuiltIn,
        const std::filesystem::path* pluginDirectory)
    {
        std::vector<apex::extension::PluginFinding> findings;
        if (enableBuiltIn)
        {
            const apex::extension::ProjectAuditPluginRegistry registry;
            const auto builtIn = registry.RunBuiltInAudit(project, robot, world);
            findings.insert(findings.end(), builtIn.begin(), builtIn.end());
        }
        if (pluginDirectory != nullptr)
        {
            const apex::extension::ExternalAuditPluginRunner externalRunner;
            const auto externalFindings = externalRunner.Run(projectFilePath, *pluginDirectory);
            findings.insert(findings.end(), externalFindings.begin(), externalFindings.end());
        }
        return findings;
    }

    int RunJobProject(const std::filesystem::path& filePath, const std::filesystem::path* configPath, const std::filesystem::path* pluginDirectory)
    {
        const auto config = LoadRuntimeConfigOrDefault(configPath);
        const apex::platform::Logger logger(config.MinimumLogLevel);
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        if (project.Job.Empty())
        {
            throw std::invalid_argument("Project does not contain a [job] section or any [segment] definitions.");
        }

        const auto robot = BuildRobotFromProject(project);
        const auto world = BuildWorldFromProject(project);
        logger.Write(apex::platform::LogLevel::Info, "Simulating job: " + project.Job.Name);

        const apex::simulation::JobSimulationEngine engine;
        const auto result = engine.Simulate(robot, world, project.Job, config.PreferredJointVelocityLimitRadPerSec);
        const auto findings = GatherPluginFindings(project, robot, world, filePath, config.EnableAuditPlugins, pluginDirectory);

        std::cout << "Job: " << result.JobName << "\n";
        std::cout << "Segments: " << result.SegmentResults.size() << "\n";
        std::cout << "Collision-free: " << (result.CollisionFree ? "YES" : "NO") << "\n";
        std::cout << "Total duration: " << result.TotalDurationSeconds << " s\n";
        std::cout << "Total path length: " << result.TotalPathLengthMeters << " m\n";
        std::cout << "Findings: " << findings.size() << "\n";
        for (const auto& segment : result.SegmentResults)
        {
            std::cout << "  - " << segment.Segment.Name << " [" << segment.Segment.StartWaypointName << " -> " << segment.Segment.GoalWaypointName
                      << "] tag=" << segment.Segment.ProcessTag << " duration=" << segment.Quality.EstimatedCycleTimeSeconds
                      << " s collision_free=" << (segment.Quality.CollisionFree ? "true" : "false") << '\n';
        }
        for (const auto& finding : findings)
        {
            std::cout << "  [" << apex::extension::ToString(finding.Severity) << "] " << finding.Message << '\n';
        }

        if (config.GenerateSegmentSummaryFiles)
        {
            const apex::visualization::SvgExporter exporter;
            std::size_t index = 0;
            for (const auto& segment : result.SegmentResults)
            {
                const std::filesystem::path svgPath = std::filesystem::current_path() / ("segment_" + std::to_string(index) + "_" + segment.Segment.Name + ".svg");
                exporter.SaveProjectTopViewSvg(robot, world, segment.Plan, svgPath, {1200, 800, 60.0, segment.Segment.Name, true, true, true});
                logger.Write(apex::platform::LogLevel::Info, "Wrote segment summary SVG: " + svgPath.string());
                ++index;
            }
        }
        return 0;
    }

    int InspectProject(const std::filesystem::path& filePath, const std::filesystem::path* pluginDirectory)
    {
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        const auto robot = BuildRobotFromProject(project);
        const auto world = BuildWorldFromProject(project);
        PrintRobotSummary(robot);
        std::cout << "Project schema: " << project.SchemaVersion << "\n";
        std::cout << "Obstacles: " << project.Obstacles.size() << "\n";
        if (!project.Job.Empty())
        {
            std::cout << "Job: " << project.Job.Name << "\n";
            std::cout << "Waypoints: " << project.Job.Waypoints.size() << " Segments: " << project.Job.Segments.size() << "\n";
        }

        const auto findings = GatherPluginFindings(project, robot, world, filePath, true, pluginDirectory);
        std::cout << "Plugin findings: " << findings.size() << "\n";
        for (const auto& finding : findings)
        {
            std::cout << "  [" << apex::extension::ToString(finding.Severity) << "] " << finding.Message << '\n';
        }
        return 0;
    }

    int ListPlugins(const std::filesystem::path* pluginDirectory)
    {
        const apex::extension::ProjectAuditPluginRegistry registry;
        const auto plugins = registry.CreateBuiltInPlugins();
        std::cout << "Built-in audit plugins:\n";
        for (const auto& plugin : plugins)
        {
            std::cout << "  - " << plugin->GetName() << '\n';
        }
        if (pluginDirectory != nullptr)
        {
            const apex::extension::ExternalAuditPluginRunner externalRunner;
            const auto manifests = externalRunner.Discover(*pluginDirectory);
            std::cout << "External plugin manifests:\n";
            for (const auto& manifest : manifests)
            {
                std::cout << "  - " << manifest.Name << " => " << manifest.ExecutablePath.string() << '\n';
            }
        }
        return 0;
    }

    int ExportDemoProject(const std::filesystem::path& filePath)
    {
        const apex::io::StudioProjectSerializer serializer;
        serializer.SaveToFile(BuildDemoProject(), filePath);
        std::cout << "Demo project exported: " << filePath.string() << "\n";
        return 0;
    }

    int ExportRuntimeConfig(const std::filesystem::path& filePath)
    {
        const apex::platform::RuntimeConfigLoader loader;
        loader.SaveDefaultFile(filePath);
        std::cout << "Runtime config exported: " << filePath.string() << "\n";
        return 0;
    }

    int ExportDemoBatch(const std::filesystem::path& batchPath, const std::filesystem::path& executableDirectory)
    {
        const auto batchDirectory = batchPath.has_parent_path() ? batchPath.parent_path() : std::filesystem::current_path();
        std::filesystem::create_directories(batchDirectory);

        const auto projectPath = batchDirectory / "demo_workcell_v08.arsproject";
        const auto runtimePath = batchDirectory / "runtime_v08.ini";
        const auto pluginDirectory = batchDirectory / "plugins";

        ExportDemoProject(projectPath);
        ExportRuntimeConfig(runtimePath);
        PreparePluginWorkspace(pluginDirectory, GetSampleAuditToolPath(executableDirectory));

        apex::ops::BatchManifest manifest;
        manifest.Name = "Demo Production Validation";
        manifest.Entries.push_back({"primary_demo", projectPath, runtimePath, pluginDirectory});
        const apex::ops::BatchManifestLoader loader;
        loader.SaveToFile(manifest, batchPath);
        std::cout << "Demo batch exported: " << batchPath.string() << "\n";
        return 0;
    }

    void EnsureProjectHasDefaultJob(apex::io::StudioProject& project)
    {
        if (!project.Job.Empty())
        {
            return;
        }
        project.Job.Name = "Imported Robot Dry Run";
        apex::job::NamedWaypoint home{"Home", {std::vector<double>(project.Joints.size(), 0.0)}};
        apex::job::NamedWaypoint approach{"Approach", {std::vector<double>(project.Joints.size(), 0.0)}};
        for (std::size_t i = 0; i < approach.State.JointAnglesRad.size(); ++i)
        {
            approach.State.JointAnglesRad[i] = (i % 2 == 0) ? apex::core::DegreesToRadians(10.0) : apex::core::DegreesToRadians(-10.0);
        }
        project.Job.Waypoints.push_back(home);
        project.Job.Waypoints.push_back(approach);
        project.Job.Segments.push_back({"ImportedMove", "Home", "Approach", 12, 2.0, "dry_run"});
        project.Job.LinkSafetyRadius = project.Motion.LinkSafetyRadius;
    }

    int ImportUrdf(const std::filesystem::path& urdfPath, const std::filesystem::path& projectPath)
    {
        const apex::importer::UrdfImporter importer;
        const auto result = importer.ImportFromFile(urdfPath);
        apex::io::StudioProject project = result.Project;
        EnsureProjectHasDefaultJob(project);
        if (result.UsedXacro)
        {
            const std::filesystem::path expandedPath = projectPath.parent_path() / (projectPath.stem().string() + std::string(".expanded.urdf"));
            std::ofstream expandedStream(expandedPath);
            expandedStream << result.ExpandedRobotXml;
            project.DescriptionSource.ExpandedDescriptionPath = expandedPath.string();
        }
        const apex::io::StudioProjectSerializer serializer;
        serializer.SaveToFile(project, projectPath);
        std::cout << (result.UsedXacro ? "Robot description imported from Xacro: " : "Robot description imported from URDF: ") << urdfPath.string() << "\n";
        std::cout << "Generated project: " << projectPath.string() << "\n";
        std::cout << "Imported revolute joints: " << result.ImportedRevoluteJointCount << " / " << result.TotalJointElements << "\n";
        std::cout << "Mesh resources captured: " << project.MeshResources.size() << "\n";
        if (result.UsedXacro && result.UsedExternalXacroTool)
        {
            std::cout << "Xacro expansion path: external xacro executable\n";
        }
        for (const auto& warning : result.Warnings)
        {
            std::cout << "  warning: " << warning << '\n';
        }
        return 0;
    }

    int InspectRobotDescription(const std::filesystem::path& projectPath, const std::filesystem::path* outputPath)
    {
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(projectPath);
        const apex::importer::RobotDescriptionInspector inspector;
        const auto inspection = inspector.Inspect(project);
        std::cout << "Robot description inspection for project: " << project.ProjectName << "\n";
        std::cout << "Source kind: " << inspection.SourceKind << "\n";
        std::cout << "Package dependencies: " << inspection.PackageDependencyCount << "\n";
        std::cout << "Mesh resources: " << inspection.MeshResourceCount << "\n";
        std::cout << "Resolved meshes: " << inspection.ResolvedMeshCount << "\n";
        std::cout << "Missing meshes: " << inspection.MissingMeshCount << "\n";
        if (outputPath != nullptr)
        {
            const auto extension = outputPath->extension().string();
            if (extension == ".json")
            {
                inspector.WriteJsonReport(inspection, *outputPath);
            }
            else
            {
                inspector.WriteHtmlReport(inspection, *outputPath);
            }
            std::cout << "Inspection report written: " << outputPath->string() << "\n";
        }
        for (const auto& note : inspection.Notes)
        {
            std::cout << "  note: " << note << '\n';
        }
        return 0;
    }

    int ExportRos2Workspace(const std::filesystem::path& projectPath, const std::filesystem::path& outputDirectory)
    {
        const apex::io::StudioProjectSerializer serializer;
        auto project = serializer.LoadFromFile(projectPath);
        EnsureProjectHasDefaultJob(project);
        const apex::integration::Ros2WorkspaceExporter exporter;
        const auto result = exporter.ExportWorkspace(project, outputDirectory);
        std::cout << "ROS2 workspace exported: " << result.WorkspaceRoot.string() << "\n";
        std::cout << "Bringup package: " << result.BringupPackageRoot.string() << "\n";
        std::cout << "MoveIt package: " << result.MoveItPackageRoot.string() << "\n";
        std::cout << "Generated files: " << result.GeneratedFiles.size() << "\n";
        return 0;
    }

    int RenderProjectSvg(const std::filesystem::path& filePath, const std::filesystem::path& outputPath)
    {
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        const auto robot = BuildRobotFromProject(project);
        const auto world = BuildWorldFromProject(project);
        const apex::visualization::SvgExporter exporter;
        if (!project.Job.Empty())
        {
            const apex::simulation::JobSimulationEngine engine;
            const auto result = engine.Simulate(robot, world, project.Job);
            exporter.SaveJobTopViewSvg(robot, world, result, outputPath, {1200, 800, 60.0, project.ProjectName + " Job", true, true, true});
        }
        else
        {
            const auto plan = PlanTrajectory(robot, world, project.Motion);
            exporter.SaveProjectTopViewSvg(robot, world, plan, outputPath, {1200, 800, 60.0, project.ProjectName, true, true, true});
        }
        std::cout << "SVG exported: " << outputPath.string() << "\n";
        return 0;
    }

    int ReportProject(const std::filesystem::path& filePath, const std::filesystem::path& outputPath)
    {
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        const auto robot = BuildRobotFromProject(project);
        const auto world = BuildWorldFromProject(project);
        const auto plan = PlanTrajectory(robot, world, project.Motion);
        const apex::analysis::TrajectoryAnalyzer analyzer;
        const auto quality = analyzer.Analyze(robot, world, plan, project.Motion.LinkSafetyRadius);
        const apex::visualization::SvgExporter svgExporter;
        const std::string svg = svgExporter.BuildProjectTopViewSvg(robot, world, plan, {1200, 800, 60.0, project.ProjectName, true, true, true});
        const apex::report::ProjectReportGenerator reportGenerator;
        reportGenerator.SaveHtmlReport(project, plan, quality, svg, outputPath);
        std::cout << "HTML report exported: " << outputPath.string() << "\n";
        return 0;
    }

    int ReportJobProject(const std::filesystem::path& filePath, const std::filesystem::path& outputPath, const std::filesystem::path* configPath, const std::filesystem::path* pluginDirectory)
    {
        const auto config = LoadRuntimeConfigOrDefault(configPath);
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        if (project.Job.Empty())
        {
            throw std::invalid_argument("Project does not contain a robot job.");
        }
        const auto robot = BuildRobotFromProject(project);
        const auto world = BuildWorldFromProject(project);
        const apex::simulation::JobSimulationEngine engine;
        const auto result = engine.Simulate(robot, world, project.Job, config.PreferredJointVelocityLimitRadPerSec);
        const auto findings = GatherPluginFindings(project, robot, world, filePath, config.EnableAuditPlugins, pluginDirectory);
        const apex::visualization::SvgExporter svgExporter;
        const std::string svg = svgExporter.BuildJobTopViewSvg(robot, world, result, {1200, 800, 60.0, project.ProjectName + " Job", true, true, true});
        const apex::report::ProjectReportGenerator reportGenerator;
        reportGenerator.SaveJobHtmlReport(project, result, findings, svg, outputPath);
        std::cout << "HTML job report exported: " << outputPath.string() << "\n";
        return 0;
    }

    int BundleProject(const std::filesystem::path& projectPath, const std::filesystem::path* runtimePath, const std::filesystem::path& outputDirectory, const std::filesystem::path& executableDirectory, const std::filesystem::path* explicitPluginDirectory)
    {
        const apex::platform::RuntimeConfigLoader loader;
        std::filesystem::path effectiveRuntimePath;
        bool removeTempRuntime = false;
        if (runtimePath != nullptr)
        {
            effectiveRuntimePath = *runtimePath;
        }
        else
        {
            effectiveRuntimePath = std::filesystem::temp_directory_path() / "apex_bundle_runtime.ini";
            loader.SaveDefaultFile(effectiveRuntimePath);
            removeTempRuntime = true;
        }

        std::filesystem::path pluginDirectory;
        const std::filesystem::path* pluginDirectoryPtr = explicitPluginDirectory;
        if (pluginDirectoryPtr == nullptr)
        {
            pluginDirectory = outputDirectory / "_generated_plugins";
            PreparePluginWorkspace(pluginDirectory, GetSampleAuditToolPath(executableDirectory));
            pluginDirectoryPtr = &pluginDirectory;
        }

        const apex::bundle::ProjectBundleManager manager;
        manager.CreateBundle(projectPath, effectiveRuntimePath, outputDirectory, pluginDirectoryPtr);
        std::cout << "Bundle created: " << outputDirectory.string() << "\n";

        if (removeTempRuntime)
        {
            std::error_code errorCode;
            std::filesystem::remove(effectiveRuntimePath, errorCode);
        }
        if (!pluginDirectory.empty())
        {
            std::error_code errorCode;
            std::filesystem::remove_all(pluginDirectory, errorCode);
        }
        return 0;
    }


    int PrintProjectTree(const std::filesystem::path& filePath)
    {
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        const apex::catalog::ProjectCatalogBuilder builder;
        const auto catalog = builder.Build(project);
        std::cout << builder.BuildTextTree(catalog);
        return 0;
    }

    int RecordJobSession(const std::filesystem::path& filePath, const std::filesystem::path& outputDirectory, const std::filesystem::path* configPath, const std::filesystem::path* pluginDirectory)
    {
        const auto config = LoadRuntimeConfigOrDefault(configPath);
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        if (project.Job.Empty())
        {
            throw std::invalid_argument("Project does not contain a robot job.");
        }
        const auto robot = BuildRobotFromProject(project);
        const auto world = BuildWorldFromProject(project);
        const apex::simulation::JobSimulationEngine engine;
        const auto result = engine.Simulate(robot, world, project.Job, config.PreferredJointVelocityLimitRadPerSec);
        const auto findings = GatherPluginFindings(project, robot, world, filePath, config.EnableAuditPlugins, pluginDirectory);

        apex::session::SessionRecorder recorder;
        recorder.SetTitle("Job execution session");
        recorder.SetProjectName(project.ProjectName);
        recorder.SetJobName(project.Job.Name);
        recorder.SetMetrics(result.TotalDurationSeconds, result.TotalPathLengthMeters, findings.size());
        recorder.AddEvent("project", "info", "Loaded project file: " + filePath.string());
        recorder.AddEvent("job", "info", "Job contains " + std::to_string(project.Job.Segments.size()) + " segments.");
        for (const auto& segment : result.SegmentResults)
        {
            recorder.AddEvent("segment", segment.Quality.CollisionFree ? "info" : "warning",
                segment.Segment.Name + " duration=" + std::to_string(segment.Quality.EstimatedCycleTimeSeconds) + "s path=" + std::to_string(segment.Quality.PathLengthMeters) + "m");
        }
        for (const auto& warning : result.Warnings)
        {
            recorder.AddWarning(warning);
        }
        for (const auto& finding : findings)
        {
            recorder.AddEvent("audit", apex::extension::ToString(finding.Severity), finding.Message);
            if (finding.Severity != apex::extension::FindingSeverity::Info)
            {
                recorder.AddWarning(finding.Message);
            }
        }
        recorder.MarkStatus(result.CollisionFree && findings.empty() ? apex::session::SessionStatus::Success : apex::session::SessionStatus::Warning);
        recorder.SaveToDirectory(outputDirectory);
        std::cout << "Session exported: " << outputDirectory.string() << "\n";
        return 0;
    }

    int DashboardProject(const std::filesystem::path& filePath, const std::filesystem::path& outputPath, const std::filesystem::path* configPath, const std::filesystem::path* pluginDirectory)
    {
        const auto config = LoadRuntimeConfigOrDefault(configPath);
        const apex::io::StudioProjectSerializer serializer;
        const auto project = serializer.LoadFromFile(filePath);
        if (project.Job.Empty())
        {
            throw std::invalid_argument("Project does not contain a robot job.");
        }
        const auto robot = BuildRobotFromProject(project);
        const auto world = BuildWorldFromProject(project);
        const apex::simulation::JobSimulationEngine engine;
        const auto result = engine.Simulate(robot, world, project.Job, config.PreferredJointVelocityLimitRadPerSec);
        const auto findings = GatherPluginFindings(project, robot, world, filePath, config.EnableAuditPlugins, pluginDirectory);
        const apex::catalog::ProjectCatalogBuilder catalogBuilder;
        const auto catalog = catalogBuilder.Build(project);
        const apex::visualization::SvgExporter svgExporter;
        const std::string svg = svgExporter.BuildJobTopViewSvg(robot, world, result, {1200, 800, 60.0, project.ProjectName + " Dashboard", true, true, true});

        apex::session::SessionRecorder recorder;
        recorder.SetTitle("Workbench dashboard session");
        recorder.SetProjectName(project.ProjectName);
        recorder.SetJobName(project.Job.Name);
        recorder.SetMetrics(result.TotalDurationSeconds, result.TotalPathLengthMeters, findings.size());
        recorder.AddEvent("dashboard", "info", "Generated project dashboard.");
        recorder.AddArtifact("dashboard", outputPath, "Interactive HTML workstation dashboard");
        for (const auto& warning : result.Warnings)
        {
            recorder.AddWarning(warning);
        }
        for (const auto& finding : findings)
        {
            recorder.AddEvent("audit", apex::extension::ToString(finding.Severity), finding.Message);
            if (finding.Severity != apex::extension::FindingSeverity::Info)
            {
                recorder.AddWarning(finding.Message);
            }
        }
        recorder.MarkStatus(result.CollisionFree && findings.empty() ? apex::session::SessionStatus::Success : apex::session::SessionStatus::Warning);

        const apex::dashboard::WorkbenchDashboardGenerator generator;
        generator.SaveProjectDashboard(project, catalog, result, findings, recorder.GetSummary(), svg, outputPath);
        std::cout << "Project dashboard exported: " << outputPath.string() << "\n";
        return 0;
    }

    int DashboardBatch(const std::filesystem::path& batchPath, const std::filesystem::path& outputPath)
    {
        const apex::ops::BatchManifestLoader loader;
        const auto manifest = loader.LoadFromFile(batchPath);
        const apex::ops::BatchRunner runner;
        const auto result = runner.Run(manifest);

        apex::session::SessionRecorder recorder;
        recorder.SetTitle("Batch dashboard session");
        recorder.SetProjectName(manifest.Name);
        recorder.SetJobName("batch_validation");
        recorder.SetMetrics(0.0, 0.0, 0);
        recorder.AddEvent("batch", "info", "Loaded batch with " + std::to_string(manifest.Entries.size()) + " entries.");
        recorder.AddEvent("batch", result.FailureCount == 0 ? "info" : "warning", "Run completed with success=" + std::to_string(result.SuccessCount) + " failure=" + std::to_string(result.FailureCount));
        recorder.AddArtifact("dashboard", outputPath, "Batch validation dashboard");
        recorder.MarkStatus(result.FailureCount == 0 ? apex::session::SessionStatus::Success : apex::session::SessionStatus::Warning);

        const apex::dashboard::WorkbenchDashboardGenerator generator;
        generator.SaveBatchDashboard(result, recorder.GetSummary(), outputPath);
        std::cout << "Batch dashboard exported: " << outputPath.string() << "\n";
        return result.FailureCount == 0 ? 0 : 1;
    }
    int RunBundle(const std::filesystem::path& bundleDirectory)
    {
        const apex::bundle::ProjectBundleManager manager;
        const auto manifest = manager.LoadManifest(bundleDirectory);
        const auto projectPath = manager.ResolveProjectPath(manifest, bundleDirectory);
        const auto runtimePath = manager.ResolveRuntimePath(manifest, bundleDirectory);
        const auto pluginDirectory = manager.ResolvePluginDirectory(manifest, bundleDirectory);
        return RunJobProject(projectPath, &runtimePath, &pluginDirectory);
    }

    int RunBatch(const std::filesystem::path& batchPath)
    {
        const apex::ops::BatchManifestLoader loader;
        const auto manifest = loader.LoadFromFile(batchPath);
        const apex::ops::BatchRunner runner;
        const auto result = runner.Run(manifest);
        std::cout << "Batch: " << result.Name << "\n";
        std::cout << "Entries: " << result.Entries.size() << " success=" << result.SuccessCount << " failure=" << result.FailureCount << "\n";
        for (const auto& entry : result.Entries)
        {
            std::cout << "  - " << entry.Entry.Name << " status=" << (entry.Success ? "OK" : "Failed")
                      << " collision_free=" << (entry.CollisionFree ? "YES" : "NO")
                      << " duration=" << entry.TotalDurationSeconds
                      << " path=" << entry.TotalPathLengthMeters
                      << " findings=" << entry.FindingCount << '\n';
            for (const auto& warning : entry.Warnings)
            {
                std::cout << "      warning: " << warning << '\n';
            }
        }
        return result.FailureCount == 0 ? 0 : 1;
    }

    int ReportBatch(const std::filesystem::path& batchPath, const std::filesystem::path& outputPath)
    {
        const apex::ops::BatchManifestLoader loader;
        const auto manifest = loader.LoadFromFile(batchPath);
        const apex::ops::BatchRunner runner;
        const auto result = runner.Run(manifest);
        runner.SaveHtmlReport(result, outputPath);
        std::cout << "Batch report exported: " << outputPath.string() << "\n";
        return result.FailureCount == 0 ? 0 : 1;
    }
}


apex::recipe::ProcessRecipe BuildDemoRecipe()
{
    apex::recipe::ProcessRecipe recipe;
    recipe.Name = "Automotive Painting Standard";
    recipe.ProcessFamily = "painting";
    recipe.Owner = "manufacturing.engineering";
    recipe.Notes = "Standardized spray-cell defaults for interview demo and product prototype.";
    recipe.DefaultSampleCount = 20;
    recipe.SegmentDurationScale = 1.15;
    recipe.LinkSafetyRadius = 0.09;
    recipe.MaxPeakTcpSpeedMetersPerSecond = 1.4;
    recipe.MaxAverageTcpSpeedMetersPerSecond = 0.9;
    recipe.PreferredJointVelocityLimitRadPerSec = 1.2;
    recipe.MaxWarningFindings = 1;
    recipe.RequireCollisionFree = true;
    recipe.ProcessTagPrefix = "paint";
    return recipe;
}

apex::edit::ProjectPatch BuildDemoPatch()
{
    apex::edit::ProjectPatch patch;
    patch.Name = "Shift Fixture And Tighten Motion";
    patch.Notes = "Moves a fixture and tightens motion sampling for validation.";
    patch.OwnerOverride = "robotics.integration";
    patch.Motion.HasSampleCount = true;
    patch.Motion.SampleCount = 22;
    patch.Motion.HasDurationSeconds = true;
    patch.Motion.DurationSeconds = 5.0;
    patch.Motion.HasLinkSafetyRadius = true;
    patch.Motion.LinkSafetyRadius = 0.10;
    patch.ObstaclesToTranslate.push_back({"Fixture_A", {0.06, -0.04, 0.0}});
    patch.ObstaclesToAdd.push_back({"ScannerStand", {0.15, 0.55, -0.10}, {0.28, 0.68, 0.85}});
    return patch;
}

int ExportDemoRecipe(const std::filesystem::path& filePath)
{
    const apex::recipe::ProcessRecipeLoader loader;
    loader.SaveToFile(BuildDemoRecipe(), filePath);
    std::cout << "Demo recipe exported: " << filePath.string() << "\n";
    return 0;
}

int ExportDemoPatch(const std::filesystem::path& filePath)
{
    const apex::edit::ProjectPatchLoader loader;
    loader.SaveToFile(BuildDemoPatch(), filePath);
    std::cout << "Demo patch exported: " << filePath.string() << "\n";
    return 0;
}

int ApplyRecipeToProject(const std::filesystem::path& projectPath, const std::filesystem::path& recipePath, const std::filesystem::path& outputPath)
{
    const apex::io::StudioProjectSerializer serializer;
    const apex::recipe::ProcessRecipeLoader loader;
    const auto project = serializer.LoadFromFile(projectPath);
    const auto recipe = loader.LoadFromFile(recipePath);
    const apex::recipe::ProcessRecipeApplicator applicator;
    const auto outputProject = applicator.ApplyRecipe(project, recipe);
    serializer.SaveToFile(outputProject, outputPath);
    std::cout << "Recipe applied: " << recipe.Name << "\n";
    std::cout << "Output project: " << outputPath.string() << "\n";
    return 0;
}

int ApplyPatchToProject(const std::filesystem::path& projectPath, const std::filesystem::path& patchPath, const std::filesystem::path& outputPath)
{
    const apex::io::StudioProjectSerializer serializer;
    const apex::edit::ProjectPatchLoader loader;
    const auto project = serializer.LoadFromFile(projectPath);
    const auto patch = loader.LoadFromFile(patchPath);
    const apex::edit::ProjectEditor editor;
    const auto outputProject = editor.ApplyPatch(project, patch);
    serializer.SaveToFile(outputProject, outputPath);
    std::cout << "Patch applied: " << patch.Name << "\n";
    std::cout << "Output project: " << outputPath.string() << "\n";
    return 0;
}

int GateProject(const std::filesystem::path& filePath, const std::filesystem::path& outputPath, const std::filesystem::path* configPath, const std::filesystem::path* pluginDirectory)
{
    const auto config = LoadRuntimeConfigOrDefault(configPath);
    const apex::io::StudioProjectSerializer serializer;
    const auto project = serializer.LoadFromFile(filePath);
    if (project.Job.Empty())
    {
        throw std::invalid_argument("Project does not contain a robot job.");
    }

    const auto robot = BuildRobotFromProject(project);
    const auto world = BuildWorldFromProject(project);
    const apex::simulation::JobSimulationEngine engine;
    const auto result = engine.Simulate(robot, world, project.Job, config.PreferredJointVelocityLimitRadPerSec);
    const auto findings = GatherPluginFindings(project, robot, world, filePath, config.EnableAuditPlugins, pluginDirectory);

    const apex::quality::ReleaseGateEvaluator evaluator;
    const auto evaluation = evaluator.Evaluate(project, result, findings);
    evaluator.SaveHtmlReport(project, evaluation, outputPath);

    std::cout << "Release gate status: " << apex::quality::ToString(evaluation.Status) << "\n";
    std::cout << "Gate report exported: " << outputPath.string() << "\n";
    return evaluation.Status == apex::quality::GateStatus::Fail ? 1 : 0;
}

int DiffProject(const std::filesystem::path& lhsPath, const std::filesystem::path& rhsPath, const std::filesystem::path* outputPath)
{
    const apex::io::StudioProjectSerializer serializer;
    const auto lhs = serializer.LoadFromFile(lhsPath);
    const auto rhs = serializer.LoadFromFile(rhsPath);

    const apex::diff::ProjectDiffEngine diffEngine;
    const auto entries = diffEngine.Compare(lhs, rhs);
    const auto report = diffEngine.BuildTextReport(entries);
    std::cout << report;
    if (outputPath != nullptr)
    {
        diffEngine.SaveTextReport(entries, *outputPath);
        std::cout << "Diff report exported: " << outputPath->string() << "\n";
    }
    return 0;
}



apex::templating::ProjectTemplate BuildDemoTemplate()
{
    apex::templating::ProjectTemplate projectTemplate;
    projectTemplate.Name = "Paint Cell Launch Template";
    projectTemplate.Description = "Reusable robot-cell starter for interview demos and workstation prototyping.";
    projectTemplate.DefaultProjectName = "Instantiated Paint Cell";
    projectTemplate.DefaultCellName = "Cell-Launch-01";
    projectTemplate.DefaultProcessFamily = "painting";
    projectTemplate.DefaultOwner = "manufacturing.engineering";
    projectTemplate.DefaultNotesSuffix = "Instantiated from governed project template.";
    projectTemplate.SampleCountOverride = 20;
    projectTemplate.DurationScale = 1.10;
    projectTemplate.LinkSafetyRadius = 0.09;
    projectTemplate.ObstacleOffset = {0.05, 0.00, 0.0};
    projectTemplate.BaseProject = BuildDemoProject();
    return projectTemplate;
}

int ExportDemoTemplate(const std::filesystem::path& filePath)
{
    const apex::templating::ProjectTemplateLoader loader;
    loader.SaveToFile(BuildDemoTemplate(), filePath);
    std::cout << "Demo template exported: " << filePath.string() << "\n";
    return 0;
}

int InstantiateTemplateProject(const std::filesystem::path& templatePath, const std::filesystem::path& outputPath, const std::string* projectNameOverride)
{
    const apex::templating::ProjectTemplateLoader loader;
    const auto projectTemplate = loader.LoadFromFile(templatePath);
    apex::templating::TemplateOverrides overrides;
    if (projectNameOverride != nullptr)
    {
        overrides.ProjectName = *projectNameOverride;
    }
    const apex::templating::ProjectTemplateInstantiator instantiator;
    const auto project = instantiator.Instantiate(projectTemplate, overrides);
    const apex::io::StudioProjectSerializer serializer;
    serializer.SaveToFile(project, outputPath);
    std::cout << "Template instantiated: " << projectTemplate.Name << "\n";
    std::cout << "Output project: " << outputPath.string() << "\n";
    return 0;
}

int RegressProject(const std::filesystem::path& baselinePath, const std::filesystem::path& candidatePath, const std::filesystem::path& outputPath, const std::filesystem::path* configPath, const std::filesystem::path* pluginDirectory)
{
    const auto config = LoadRuntimeConfigOrDefault(configPath);
    const apex::io::StudioProjectSerializer serializer;
    const auto baselineProject = serializer.LoadFromFile(baselinePath);
    const auto candidateProject = serializer.LoadFromFile(candidatePath);
    if (baselineProject.Job.Empty() || candidateProject.Job.Empty())
    {
        throw std::invalid_argument("Both baseline and candidate projects must contain robot jobs.");
    }

    const apex::simulation::JobSimulationEngine engine;

    const auto baselineRobot = BuildRobotFromProject(baselineProject);
    const auto baselineWorld = BuildWorldFromProject(baselineProject);
    const auto baselineSimulation = engine.Simulate(baselineRobot, baselineWorld, baselineProject.Job, config.PreferredJointVelocityLimitRadPerSec);
    const auto baselineFindings = GatherPluginFindings(baselineProject, baselineRobot, baselineWorld, baselinePath, config.EnableAuditPlugins, pluginDirectory);

    const auto candidateRobot = BuildRobotFromProject(candidateProject);
    const auto candidateWorld = BuildWorldFromProject(candidateProject);
    const auto candidateSimulation = engine.Simulate(candidateRobot, candidateWorld, candidateProject.Job, config.PreferredJointVelocityLimitRadPerSec);
    const auto candidateFindings = GatherPluginFindings(candidateProject, candidateRobot, candidateWorld, candidatePath, config.EnableAuditPlugins, pluginDirectory);

    const apex::quality::ReleaseGateEvaluator gateEvaluator;
    const auto baselineGate = gateEvaluator.Evaluate(baselineProject, baselineSimulation, baselineFindings);
    const auto candidateGate = gateEvaluator.Evaluate(candidateProject, candidateSimulation, candidateFindings);

    const apex::regression::ProjectRegressionAnalyzer analyzer;
    const auto result = analyzer.Evaluate(baselineSimulation, baselineGate, candidateSimulation, candidateGate);
    analyzer.SaveHtmlReport(baselineProject, candidateProject, result, outputPath);

    std::cout << "Regression status: " << apex::regression::ToString(result.Status) << "\n";
    std::cout << "Regression report exported: " << outputPath.string() << "\n";
    return result.Status == apex::regression::RegressionStatus::Regressed ? 1 : 0;
}

int CreateDeliveryDossier(const std::filesystem::path& filePath, const std::filesystem::path& outputDirectory, const std::filesystem::path* configPath, const std::filesystem::path* pluginDirectory)
{
    const auto config = LoadRuntimeConfigOrDefault(configPath);
    const apex::io::StudioProjectSerializer serializer;
    const auto project = serializer.LoadFromFile(filePath);
    if (project.Job.Empty())
    {
        throw std::invalid_argument("Project does not contain a robot job.");
    }

    std::filesystem::create_directories(outputDirectory / "input");
    std::filesystem::create_directories(outputDirectory / "config");
    std::filesystem::create_directories(outputDirectory / "reports");
    std::filesystem::create_directories(outputDirectory / "session");

    const auto robot = BuildRobotFromProject(project);
    const auto world = BuildWorldFromProject(project);
    const apex::simulation::JobSimulationEngine engine;
    const auto result = engine.Simulate(robot, world, project.Job, config.PreferredJointVelocityLimitRadPerSec);
    const auto findings = GatherPluginFindings(project, robot, world, filePath, config.EnableAuditPlugins, pluginDirectory);

    const std::filesystem::path copiedProjectPath = outputDirectory / "input" / filePath.filename();
    std::filesystem::copy_file(filePath, copiedProjectPath, std::filesystem::copy_options::overwrite_existing);
    const std::filesystem::path runtimePath = outputDirectory / "config" / "runtime.ini";
    if (configPath != nullptr)
    {
        std::filesystem::copy_file(*configPath, runtimePath, std::filesystem::copy_options::overwrite_existing);
    }
    else
    {
        apex::platform::RuntimeConfigLoader().SaveDefaultFile(runtimePath);
    }

    apex::session::SessionRecorder recorder;
    recorder.SetTitle("Delivery dossier session");
    recorder.SetProjectName(project.ProjectName);
    recorder.SetJobName(project.Job.Name);
    recorder.SetMetrics(result.TotalDurationSeconds, result.TotalPathLengthMeters, findings.size());
    recorder.AddEvent("delivery", "info", "Prepared delivery dossier workspace.");
    recorder.AddEvent("delivery", "info", "Copied project and runtime configuration.");
    for (const auto& warning : result.Warnings)
    {
        recorder.AddWarning(warning);
    }
    for (const auto& finding : findings)
    {
        recorder.AddEvent("audit", apex::extension::ToString(finding.Severity), finding.Message);
        if (finding.Severity != apex::extension::FindingSeverity::Info)
        {
            recorder.AddWarning(finding.Message);
        }
    }
    recorder.MarkStatus(result.CollisionFree && findings.empty() ? apex::session::SessionStatus::Success : apex::session::SessionStatus::Warning);
    recorder.SaveToDirectory(outputDirectory / "session");

    const apex::catalog::ProjectCatalogBuilder catalogBuilder;
    const auto catalog = catalogBuilder.Build(project);
    const apex::visualization::SvgExporter svgExporter;
    const std::string svg = svgExporter.BuildJobTopViewSvg(robot, world, result, {1200, 800, 60.0, project.ProjectName + " Delivery Dossier", true, true, true});
    const std::filesystem::path svgPath = outputDirectory / "reports" / "project_layout.svg";
    {
        std::ofstream svgStream(svgPath);
        if (!svgStream)
        {
            throw std::runtime_error("Failed to write SVG artifact: " + svgPath.string());
        }
        svgStream << svg;
    }

    const std::filesystem::path dashboardPath = outputDirectory / "reports" / "workstation_dashboard.html";
    apex::dashboard::WorkbenchDashboardGenerator().SaveProjectDashboard(project, catalog, result, findings, recorder.GetSummary(), svg, dashboardPath);

    const apex::quality::ReleaseGateEvaluator gateEvaluator;
    const auto gate = gateEvaluator.Evaluate(project, result, findings);
    const std::filesystem::path gatePath = outputDirectory / "reports" / "release_gate.html";
    gateEvaluator.SaveHtmlReport(project, gate, gatePath);

    const std::filesystem::path jobReportPath = outputDirectory / "reports" / "job_report.html";
    apex::report::ProjectReportGenerator().SaveJobHtmlReport(project, result, findings, svg, jobReportPath);

    if (pluginDirectory != nullptr && std::filesystem::exists(*pluginDirectory))
    {
        const std::filesystem::path copiedPluginDir = outputDirectory / "input" / "plugins";
        std::filesystem::create_directories(copiedPluginDir);
        std::filesystem::copy(*pluginDirectory, copiedPluginDir, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
    }

    apex::delivery::DeliveryDossierManifest manifest;
    manifest.Title = "ApexRoboticsStudio Delivery Dossier";
    manifest.ProjectName = project.ProjectName;
    manifest.GateStatus = gate.Status;
    manifest.SessionStatus = recorder.GetSummary().Status;
    manifest.Artifacts.push_back({"project", std::filesystem::path("input") / filePath.filename(), "Copied project input"});
    manifest.Artifacts.push_back({"config", std::filesystem::path("config/runtime.ini"), "Runtime configuration used for validation"});
    manifest.Artifacts.push_back({"dashboard", std::filesystem::path("reports/workstation_dashboard.html"), "Workbench dashboard"});
    manifest.Artifacts.push_back({"gate", std::filesystem::path("reports/release_gate.html"), "Release gate evaluation"});
    manifest.Artifacts.push_back({"report", std::filesystem::path("reports/job_report.html"), "Detailed job report"});
    manifest.Artifacts.push_back({"svg", std::filesystem::path("reports/project_layout.svg"), "Top-view layout and TCP path"});
    manifest.Artifacts.push_back({"session", std::filesystem::path("session/session_summary.html"), "Execution session summary"});
    manifest.Artifacts.push_back({"session", std::filesystem::path("session/session_summary.json"), "Execution session JSON summary"});

    const apex::delivery::DeliveryDossierBuilder builder;
    builder.SaveManifestJson(manifest, outputDirectory / "delivery_manifest.json");
    builder.SaveIndexHtml(manifest, outputDirectory / "index.html");

    std::cout << "Delivery dossier created: " << outputDirectory.string() << "\n";
    return gate.Status == apex::quality::GateStatus::Fail ? 1 : 0;
}


int ExportApprovalTemplate(const std::filesystem::path& filePath)
{
    const apex::governance::ProjectApprovalWorkflow workflow;
    const auto document = workflow.CreateTemplate(BuildDemoProject());
    apex::governance::ProjectApprovalSerializer().SaveToFile(document, filePath);
    std::cout << "Approval template exported: " << filePath.string() << "\n";
    return 0;
}

int SignProjectApproval(const std::filesystem::path& projectPath, const std::filesystem::path& approvalPath, const std::string& role, const std::string& signer, const std::string& decisionText, const std::string& notes)
{
    const apex::io::StudioProjectSerializer serializer;
    const auto project = serializer.LoadFromFile(projectPath);
    apex::governance::ProjectApprovalSerializer approvalSerializer;
    apex::governance::ProjectApprovalDocument document;
    if (std::filesystem::exists(approvalPath))
    {
        document = approvalSerializer.LoadFromFile(approvalPath);
    }
    else
    {
        document = apex::governance::ProjectApprovalWorkflow().CreateTemplate(project);
    }

    apex::governance::ProjectApprovalWorkflow workflow;
    workflow.Sign(project, document, role, signer, apex::governance::ApprovalDecisionFromString(decisionText), notes);
    approvalSerializer.SaveToFile(document, approvalPath);
    std::cout << "Project approval updated: " << approvalPath.string() << "\n";
    return 0;
}

int VerifyProjectApproval(const std::filesystem::path& projectPath, const std::filesystem::path& approvalPath, const std::filesystem::path& outputPath)
{
    const apex::io::StudioProjectSerializer serializer;
    const auto project = serializer.LoadFromFile(projectPath);
    const auto document = apex::governance::ProjectApprovalSerializer().LoadFromFile(approvalPath);
    const apex::governance::ProjectApprovalWorkflow workflow;
    const auto verification = workflow.Verify(project, document);
    workflow.SaveHtmlReport(project, document, verification, outputPath);
    std::cout << "Approval decision: " << apex::governance::ToString(verification.OverallDecision) << "\n";
    std::cout << "Approval report exported: " << outputPath.string() << "\n";
    return verification.OverallDecision == apex::governance::ApprovalDecision::Rejected ? 1 : 0;
}

int CreateProjectSnapshot(const std::filesystem::path& projectPath, const std::filesystem::path& outputDirectory, const std::filesystem::path* runtimePath, const std::filesystem::path* approvalPath)
{
    const apex::revision::ProjectSnapshotManager manager;
    const auto manifest = manager.CreateSnapshot(projectPath, outputDirectory, runtimePath, approvalPath);
    std::cout << "Snapshot created: " << outputDirectory.string() << "\n";
    std::cout << "Snapshot id: " << manifest.SnapshotId << "\n";
    return 0;
}

int RestoreProjectSnapshot(const std::filesystem::path& snapshotDirectory, const std::filesystem::path& outputProjectPath, const std::filesystem::path* outputRuntimePath, const std::filesystem::path* outputApprovalPath)
{
    apex::revision::ProjectSnapshotManager().RestoreSnapshot(snapshotDirectory, outputProjectPath, outputRuntimePath, outputApprovalPath);
    std::cout << "Snapshot restored to project: " << outputProjectPath.string() << "\n";
    return 0;
}

int ExportIntegrationPackage(const std::filesystem::path& projectPath, const std::filesystem::path& outputDirectory, const std::filesystem::path* runtimePath, const std::filesystem::path* pluginDirectory, const std::filesystem::path* approvalPath)
{
    const auto config = LoadRuntimeConfigOrDefault(runtimePath);
    const apex::io::StudioProjectSerializer serializer;
    const auto project = serializer.LoadFromFile(projectPath);
    if (project.Job.Empty())
    {
        throw std::invalid_argument("Project does not contain a robot job.");
    }

    const auto robot = BuildRobotFromProject(project);
    const auto world = BuildWorldFromProject(project);
    const auto findings = GatherPluginFindings(project, robot, world, projectPath, config.EnableAuditPlugins, pluginDirectory);
    const auto simulation = apex::simulation::JobSimulationEngine().Simulate(robot, world, project.Job, config.PreferredJointVelocityLimitRadPerSec);
    const auto gate = apex::quality::ReleaseGateEvaluator().Evaluate(project, simulation, findings);

    std::optional<apex::governance::ProjectApprovalVerification> approvalVerification;
    if (approvalPath != nullptr && std::filesystem::exists(*approvalPath))
    {
        const auto document = apex::governance::ProjectApprovalSerializer().LoadFromFile(*approvalPath);
        approvalVerification = apex::governance::ProjectApprovalWorkflow().Verify(project, document);
    }

    const auto summary = apex::integration::IntegrationPackageExporter().Export(project, simulation, gate, findings, outputDirectory, approvalVerification ? &*approvalVerification : nullptr);
    std::cout << "Integration package exported: " << outputDirectory.string() << "\n";
    std::cout << "Gate: " << apex::quality::ToString(summary.GateStatus) << " Approval: " << apex::governance::ToString(summary.ApprovalDecision) << "\n";
    return gate.Status == apex::quality::GateStatus::Fail ? 1 : 0;
}

int ExportDemoFlow(const std::filesystem::path& filePath, const std::filesystem::path& executableDirectory)
{
    const auto baseDirectory = EnsureDirectoryOrCurrent(filePath.parent_path());
    std::filesystem::create_directories(baseDirectory);
    const auto projectPath = baseDirectory / "demo_flow_project_v11.arsproject";
    const auto runtimePath = baseDirectory / "demo_flow_runtime_v11.ini";
    const auto recipePath = baseDirectory / "demo_flow_recipe_v11.arsrecipe";
    const auto patchPath = baseDirectory / "demo_flow_patch_v11.arspatch";
    const auto approvalPath = baseDirectory / "demo_flow_approval_v11.arsapproval";
    const auto pluginDirectory = baseDirectory / "plugins";
    const auto outputDirectory = baseDirectory / "demo_flow_workspace_v11";

    apex::io::StudioProjectSerializer().SaveToFile(BuildDemoProject(), projectPath);
    apex::platform::RuntimeConfigLoader().SaveDefaultFile(runtimePath);
    ExportDemoRecipe(recipePath);
    ExportDemoPatch(patchPath);
    ExportApprovalTemplate(approvalPath);
    PreparePluginWorkspace(pluginDirectory, GetSampleAuditToolPath(executableDirectory));

    apex::workflow::AutomationFlow flow;
    flow.Name = "Demo Interview Automation Flow";
    flow.BaseProjectPath = projectPath.filename();
    flow.RuntimePath = runtimePath.filename();
    flow.RecipePath = recipePath.filename();
    flow.PatchPath = patchPath.filename();
    flow.ApprovalPath = approvalPath.filename();
    flow.PluginDirectory = pluginDirectory.filename();
    flow.OutputDirectory = outputDirectory.filename();
    flow.Steps.push_back({"ApplyRecipe", "apply_recipe", ""});
    flow.Steps.push_back({"ApplyPatch", "apply_patch", ""});
    flow.Steps.push_back({"GateCandidate", "gate_project", "reports/flow_gate.html"});
    flow.Steps.push_back({"DashboardCandidate", "dashboard_project", "reports/flow_dashboard.html"});
    flow.Steps.push_back({"CreateSnapshot", "snapshot_project", "snapshots/after_flow"});
    flow.Steps.push_back({"CreateDelivery", "create_delivery_dossier", "delivery"});
    flow.Steps.push_back({"ExportIntegration", "export_integration_package", "integration"});
    apex::workflow::AutomationFlowSerializer().SaveToFile(flow, filePath);
    std::cout << "Automation flow exported: " << filePath.string() << "\n";
    return 0;
}

int RunFlow(const std::filesystem::path& flowPath)
{
    const auto flow = apex::workflow::AutomationFlowSerializer().LoadFromFile(flowPath);
    const auto resolvedProjectPath = ResolveRelativeTo(flowPath, flow.BaseProjectPath);
    const auto resolvedRuntimePath = flow.RuntimePath.empty() ? std::filesystem::path{} : ResolveRelativeTo(flowPath, flow.RuntimePath);
    const auto resolvedRecipePath = flow.RecipePath.empty() ? std::filesystem::path{} : ResolveRelativeTo(flowPath, flow.RecipePath);
    const auto resolvedPatchPath = flow.PatchPath.empty() ? std::filesystem::path{} : ResolveRelativeTo(flowPath, flow.PatchPath);
    const auto resolvedApprovalPath = flow.ApprovalPath.empty() ? std::filesystem::path{} : ResolveRelativeTo(flowPath, flow.ApprovalPath);
    const auto resolvedPluginDirectory = flow.PluginDirectory.empty() ? std::filesystem::path{} : ResolveRelativeTo(flowPath, flow.PluginDirectory);
    const auto outputDirectory = ResolveRelativeTo(flowPath, flow.OutputDirectory.empty() ? std::filesystem::path{"flow_output_v11"} : flow.OutputDirectory);
    std::filesystem::create_directories(outputDirectory / "working");

    apex::trace::AuditTrailRecorder audit;
    audit.SetTitle(flow.Name);
    const apex::io::StudioProjectSerializer serializer;
    apex::io::StudioProject project = serializer.LoadFromFile(resolvedProjectPath);
    audit.SetProjectName(project.ProjectName);
    audit.SetProjectFingerprint(apex::governance::ProjectFingerprintBuilder().Build(project));
    audit.AddEvent("flow", "info", "load_project", "Loaded base project from " + resolvedProjectPath.string());

    std::filesystem::path currentProjectPath = outputDirectory / "working" / "current_project.arsproject";
    serializer.SaveToFile(project, currentProjectPath);
    audit.AddArtifact("project", std::filesystem::path("working/current_project.arsproject"), "Current project state");

    const std::filesystem::path* runtimePointer = resolvedRuntimePath.empty() ? nullptr : &resolvedRuntimePath;
    const std::filesystem::path* pluginPointer = resolvedPluginDirectory.empty() ? nullptr : &resolvedPluginDirectory;
    int finalCode = 0;

    for (const auto& step : flow.Steps)
    {
        if (step.Action == "apply_recipe")
        {
            if (resolvedRecipePath.empty())
            {
                throw std::invalid_argument("Flow step apply_recipe requires a recipe path.");
            }
            ApplyRecipeToProject(currentProjectPath, resolvedRecipePath, currentProjectPath);
            project = serializer.LoadFromFile(currentProjectPath);
            audit.SetProjectFingerprint(apex::governance::ProjectFingerprintBuilder().Build(project));
            audit.AddEvent("flow", "info", step.Action, "Applied recipe to working project.");
        }
        else if (step.Action == "apply_patch")
        {
            if (resolvedPatchPath.empty())
            {
                throw std::invalid_argument("Flow step apply_patch requires a patch path.");
            }
            ApplyPatchToProject(currentProjectPath, resolvedPatchPath, currentProjectPath);
            project = serializer.LoadFromFile(currentProjectPath);
            audit.SetProjectFingerprint(apex::governance::ProjectFingerprintBuilder().Build(project));
            audit.AddEvent("flow", "info", step.Action, "Applied patch to working project.");
        }
        else if (step.Action == "gate_project")
        {
            const auto out = outputDirectory / (step.Argument.empty() ? "reports/flow_gate.html" : std::filesystem::path(step.Argument));
            std::filesystem::create_directories(out.parent_path());
            finalCode = std::max(finalCode, GateProject(currentProjectPath, out, runtimePointer, pluginPointer));
            audit.AddArtifact("gate", std::filesystem::relative(out, outputDirectory), "Release gate report");
            audit.AddEvent("flow", finalCode == 0 ? "info" : "warning", step.Action, "Generated release gate report.");
        }
        else if (step.Action == "dashboard_project")
        {
            const auto out = outputDirectory / (step.Argument.empty() ? "reports/flow_dashboard.html" : std::filesystem::path(step.Argument));
            std::filesystem::create_directories(out.parent_path());
            DashboardProject(currentProjectPath, out, runtimePointer, pluginPointer);
            audit.AddArtifact("dashboard", std::filesystem::relative(out, outputDirectory), "Workbench dashboard");
            audit.AddEvent("flow", "info", step.Action, "Generated dashboard report.");
        }
        else if (step.Action == "create_delivery_dossier")
        {
            const auto out = outputDirectory / (step.Argument.empty() ? "delivery" : std::filesystem::path(step.Argument));
            finalCode = std::max(finalCode, CreateDeliveryDossier(currentProjectPath, out, runtimePointer, pluginPointer));
            audit.AddArtifact("delivery", std::filesystem::relative(out / "index.html", outputDirectory), "Delivery dossier index");
            audit.AddEvent("flow", finalCode == 0 ? "info" : "warning", step.Action, "Created delivery dossier.");
        }
        else if (step.Action == "export_integration_package")
        {
            const auto out = outputDirectory / (step.Argument.empty() ? "integration" : std::filesystem::path(step.Argument));
            const std::filesystem::path* approvalPointer = resolvedApprovalPath.empty() ? nullptr : &resolvedApprovalPath;
            finalCode = std::max(finalCode, ExportIntegrationPackage(currentProjectPath, out, runtimePointer, pluginPointer, approvalPointer));
            audit.AddArtifact("integration", std::filesystem::relative(out / "summary.json", outputDirectory), "Integration package summary");
            audit.AddEvent("flow", finalCode == 0 ? "info" : "warning", step.Action, "Exported integration package.");
        }
        else if (step.Action == "snapshot_project")
        {
            const auto out = outputDirectory / (step.Argument.empty() ? "snapshots/current" : std::filesystem::path(step.Argument));
            const std::filesystem::path* approvalPointer = resolvedApprovalPath.empty() ? nullptr : &resolvedApprovalPath;
            CreateProjectSnapshot(currentProjectPath, out, runtimePointer, approvalPointer);
            audit.AddArtifact("snapshot", std::filesystem::relative(out / "index.html", outputDirectory), "Snapshot index");
            audit.AddEvent("flow", "info", step.Action, "Created project snapshot.");
        }
        else
        {
            throw std::invalid_argument("Unknown flow action: " + step.Action);
        }
    }

    audit.SetOverallStatus(finalCode == 0 ? "success" : "warning");
    audit.SaveToDirectory(outputDirectory / "audit");
    std::cout << "Automation flow completed: " << outputDirectory.string() << "\n";
    return finalCode;
}

int ExportDemoSweep(const std::filesystem::path& filePath)
{
    const auto baseDirectory = EnsureDirectoryOrCurrent(filePath.parent_path());
    std::filesystem::create_directories(baseDirectory);
    const auto projectPath = baseDirectory / "demo_sweep_project_v11.arsproject";
    const auto runtimePath = baseDirectory / "demo_sweep_runtime_v11.ini";
    apex::io::StudioProjectSerializer().SaveToFile(BuildDemoProject(), projectPath);
    apex::platform::RuntimeConfigLoader().SaveDefaultFile(runtimePath);

    apex::sweep::SweepDefinition sweep;
    sweep.Name = "Cycle Time and Safety Radius Sweep";
    sweep.BaseProjectPath = projectPath.filename();
    sweep.RuntimePath = runtimePath.filename();
    sweep.OutputDirectory = std::filesystem::path("demo_sweep_workspace_v11");
    sweep.SampleCounts = {12, 16, 20};
    sweep.DurationScales = {0.85, 1.00, 1.15};
    sweep.SafetyRadii = {0.06, 0.08};
    apex::sweep::ParameterSweepSerializer().SaveToFile(sweep, filePath);
    std::cout << "Parameter sweep exported: " << filePath.string() << "\n";
    return 0;
}

int RunSweep(const std::filesystem::path& sweepPath)
{
    const auto sweep = apex::sweep::ParameterSweepSerializer().LoadFromFile(sweepPath);
    const auto projectPath = ResolveRelativeTo(sweepPath, sweep.BaseProjectPath);
    const auto runtimePath = sweep.RuntimePath.empty() ? std::filesystem::path{} : ResolveRelativeTo(sweepPath, sweep.RuntimePath);
    const auto pluginDirectory = sweep.PluginDirectory.empty() ? std::filesystem::path{} : ResolveRelativeTo(sweepPath, sweep.PluginDirectory);
    const auto outputDirectory = ResolveRelativeTo(sweepPath, sweep.OutputDirectory.empty() ? std::filesystem::path{"sweep_output_v11"} : sweep.OutputDirectory);
    std::filesystem::create_directories(outputDirectory / "variants");

    const apex::io::StudioProjectSerializer serializer;
    const auto baseProject = serializer.LoadFromFile(projectPath);
    const auto config = LoadRuntimeConfigOrDefault(runtimePath.empty() ? nullptr : &runtimePath);
    const std::filesystem::path* pluginPointer = pluginDirectory.empty() ? nullptr : &pluginDirectory;
    std::vector<apex::sweep::SweepVariantSummary> variants;
    apex::trace::AuditTrailRecorder audit;
    audit.SetTitle(sweep.Name);
    audit.SetProjectName(baseProject.ProjectName);
    audit.SetProjectFingerprint(apex::governance::ProjectFingerprintBuilder().Build(baseProject));

    for (const std::size_t sampleCount : sweep.SampleCounts)
    {
        for (const double durationScale : sweep.DurationScales)
        {
            for (const double safetyRadius : sweep.SafetyRadii)
            {
                auto variantProject = baseProject;
                variantProject.ProjectName = baseProject.ProjectName + " | sweep s=" + std::to_string(sampleCount) + " d=" + std::to_string(durationScale) + " r=" + std::to_string(safetyRadius);
                variantProject.Motion.SampleCount = sampleCount;
                variantProject.Motion.DurationSeconds = baseProject.Motion.DurationSeconds * durationScale;
                variantProject.Motion.LinkSafetyRadius = safetyRadius;
                variantProject.Job.LinkSafetyRadius = safetyRadius;
                for (auto& segment : variantProject.Job.Segments)
                {
                    segment.SampleCount = std::max<std::size_t>(4, sampleCount);
                    segment.DurationSeconds *= durationScale;
                }

                const std::string stem = "variant_sc" + std::to_string(sampleCount) + "_ds" + std::to_string(static_cast<int>(durationScale * 100.0)) + "_sr" + std::to_string(static_cast<int>(safetyRadius * 100.0));
                const auto variantProjectPath = outputDirectory / "variants" / (stem + ".arsproject");
                serializer.SaveToFile(variantProject, variantProjectPath);

                const auto robot = BuildRobotFromProject(variantProject);
                const auto world = BuildWorldFromProject(variantProject);
                const auto findings = GatherPluginFindings(variantProject, robot, world, variantProjectPath, config.EnableAuditPlugins, pluginPointer);
                const auto simulation = apex::simulation::JobSimulationEngine().Simulate(robot, world, variantProject.Job, config.PreferredJointVelocityLimitRadPerSec);
                const auto gate = apex::quality::ReleaseGateEvaluator().Evaluate(variantProject, simulation, findings);

                apex::sweep::SweepVariantSummary summary;
                summary.VariantName = stem;
                summary.SampleCount = sampleCount;
                summary.DurationScale = durationScale;
                summary.SafetyRadius = safetyRadius;
                summary.GateStatus = apex::quality::ToString(gate.Status);
                summary.CollisionFree = simulation.CollisionFree;
                summary.FindingCount = findings.size();
                summary.TotalDurationSeconds = simulation.TotalDurationSeconds;
                summary.TotalPathLengthMeters = simulation.TotalPathLengthMeters;
                variants.push_back(summary);
                audit.AddArtifact("variant", std::filesystem::relative(variantProjectPath, outputDirectory), "Generated sweep variant project");
                audit.AddEvent("sweep", gate.Status == apex::quality::GateStatus::Fail ? "warning" : "info", "evaluate_variant", stem + " -> gate=" + summary.GateStatus);
            }
        }
    }

    const apex::sweep::ParameterSweepReportBuilder builder;
    builder.SaveCsv(variants, outputDirectory / "sweep_summary.csv");
    builder.SaveHtml(sweep, variants, outputDirectory / "sweep_summary.html");
    audit.AddArtifact("report", "sweep_summary.csv", "Sweep CSV summary");
    audit.AddArtifact("report", "sweep_summary.html", "Sweep HTML summary");
    bool anyFail = false;
    for (const auto& variant : variants)
    {
        if (variant.GateStatus == "fail")
        {
            anyFail = true;
            break;
        }
    }
    audit.SetOverallStatus(anyFail ? "warning" : "success");
    audit.SaveToDirectory(outputDirectory / "audit");
    std::cout << "Parameter sweep completed: " << outputDirectory.string() << "\n";
    return anyFail ? 1 : 0;
}

int AuditProject(const std::filesystem::path& projectPath, const std::filesystem::path& outputDirectory, const std::filesystem::path* runtimePath, const std::filesystem::path* pluginDirectory, const std::filesystem::path* approvalPath)
{
    const auto config = LoadRuntimeConfigOrDefault(runtimePath);
    const apex::io::StudioProjectSerializer serializer;
    const auto project = serializer.LoadFromFile(projectPath);
    const auto robot = BuildRobotFromProject(project);
    const auto world = BuildWorldFromProject(project);
    const auto findings = GatherPluginFindings(project, robot, world, projectPath, config.EnableAuditPlugins, pluginDirectory);

    std::filesystem::create_directories(outputDirectory);
    apex::trace::AuditTrailRecorder audit;
    audit.SetTitle("Project audit trail");
    audit.SetProjectName(project.ProjectName);
    audit.SetProjectFingerprint(apex::governance::ProjectFingerprintBuilder().Build(project));
    audit.AddEvent("audit", "info", "load_project", "Loaded project for audit review.");
    audit.AddEvent("audit", "info", "plugin_audit", "Collected " + std::to_string(findings.size()) + " audit findings.");

    int exitCode = 0;
    if (!project.Job.Empty())
    {
        const auto simulation = apex::simulation::JobSimulationEngine().Simulate(robot, world, project.Job, config.PreferredJointVelocityLimitRadPerSec);
        const auto gate = apex::quality::ReleaseGateEvaluator().Evaluate(project, simulation, findings);
        const auto gatePath = outputDirectory / "release_gate.html";
        apex::quality::ReleaseGateEvaluator().SaveHtmlReport(project, gate, gatePath);
        audit.AddArtifact("gate", "release_gate.html", "Release gate evaluation");
        audit.AddEvent("audit", gate.Status == apex::quality::GateStatus::Fail ? "error" : "info", "release_gate", "Gate status: " + apex::quality::ToString(gate.Status));
        exitCode = gate.Status == apex::quality::GateStatus::Fail ? 1 : 0;
    }

    if (approvalPath != nullptr && std::filesystem::exists(*approvalPath))
    {
        const auto document = apex::governance::ProjectApprovalSerializer().LoadFromFile(*approvalPath);
        const auto verification = apex::governance::ProjectApprovalWorkflow().Verify(project, document);
        const auto approvalReport = outputDirectory / "approval_report.html";
        apex::governance::ProjectApprovalWorkflow().SaveHtmlReport(project, document, verification, approvalReport);
        audit.AddArtifact("approval", "approval_report.html", "Approval verification report");
        audit.AddEvent("audit", verification.OverallDecision == apex::governance::ApprovalDecision::Rejected ? "error" : "info", "approval_verification", "Approval decision: " + apex::governance::ToString(verification.OverallDecision));
        if (verification.OverallDecision == apex::governance::ApprovalDecision::Rejected)
        {
            exitCode = 1;
        }
    }

    const auto dashboardPath = outputDirectory / "dashboard.html";
    DashboardProject(projectPath, dashboardPath, runtimePath, pluginDirectory);
    audit.AddArtifact("dashboard", "dashboard.html", "Workbench dashboard snapshot");
    for (const auto& finding : findings)
    {
        audit.AddEvent("finding", apex::extension::ToString(finding.Severity), "project_finding", finding.Message);
    }
    audit.SetOverallStatus(exitCode == 0 ? "success" : "warning");
    audit.SaveToDirectory(outputDirectory);
    std::cout << "Audit trail exported: " << outputDirectory.string() << "\n";
    return exitCode;
}

int main(int argc, char* argv[])
{
    try
    {
        PrintBanner();
        if (argc <= 1)
        {
            PrintUsage();
            return 0;
        }

        const std::filesystem::path executableDirectory = GetExecutablePath(argv[0]).parent_path();
        const std::string command = argv[1];
        if (command == "demo")
        {
            return ExecutePlan(BuildDemoProject());
        }
        if (command == "summary")
        {
            PrintRobotSummary(BuildRobotFromProject(BuildDemoProject()));
            return 0;
        }
        if (command == "export-demo-project")
        {
            const std::filesystem::path outputPath = argc >= 3 ? std::filesystem::path(argv[2]) : (std::filesystem::current_path() / "demo_workcell_v08.arsproject");
            return ExportDemoProject(outputPath);
        }
        if (command == "export-runtime-config")
        {
            const std::filesystem::path outputPath = argc >= 3 ? std::filesystem::path(argv[2]) : (std::filesystem::current_path() / "runtime_v08.ini");
            return ExportRuntimeConfig(outputPath);
        }
        if (command == "export-demo-batch")
        {
            const std::filesystem::path outputPath = argc >= 3 ? std::filesystem::path(argv[2]) : (std::filesystem::current_path() / "demo_validation.arsbatch");
            return ExportDemoBatch(outputPath, executableDirectory);
        }
        if (command == "run-project" && argc >= 3)
        {
            return RunProject(argv[2]);
        }
        if (command == "run-job-project" && argc >= 3)
        {
            const std::filesystem::path* configPath = argc >= 4 ? new std::filesystem::path(argv[3]) : nullptr;
            const std::filesystem::path* pluginDirectory = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
            const int code = RunJobProject(argv[2], configPath, pluginDirectory);
            delete configPath;
            delete pluginDirectory;
            return code;
        }
        if (command == "inspect-project" && argc >= 3)
        {
            const std::filesystem::path* pluginDirectory = argc >= 4 ? new std::filesystem::path(argv[3]) : nullptr;
            const int code = InspectProject(argv[2], pluginDirectory);
            delete pluginDirectory;
            return code;
        }
        if (command == "list-plugins")
        {
            const std::filesystem::path* pluginDirectory = argc >= 3 ? new std::filesystem::path(argv[2]) : nullptr;
            const int code = ListPlugins(pluginDirectory);
            delete pluginDirectory;
            return code;
        }
        if ((command == "import-urdf" || command == "import-robot-description") && argc >= 3)
        {
            const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "imported_robot.arsproject");
            return ImportUrdf(argv[2], outputPath);
        }
        if (command == "inspect-robot-description" && argc >= 3)
        {
            const std::filesystem::path* outputPath = argc >= 4 ? new std::filesystem::path(argv[3]) : nullptr;
            const int code = InspectRobotDescription(argv[2], outputPath);
            delete outputPath;
            return code;
        }
        if (command == "render-project-svg" && argc >= 3)
        {
            const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "project.svg");
            return RenderProjectSvg(argv[2], outputPath);
        }
        if (command == "report-project" && argc >= 3)
        {
            const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "project_report.html");
            return ReportProject(argv[2], outputPath);
        }
        if (command == "report-job-project" && argc >= 3)
        {
            const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "job_report.html");
            const std::filesystem::path* configPath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
            const std::filesystem::path* pluginDirectory = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
            const int code = ReportJobProject(argv[2], outputPath, configPath, pluginDirectory);
            delete configPath;
            delete pluginDirectory;
            return code;
        }
        if (command == "bundle-project" && argc >= 3)
        {
            const std::filesystem::path* runtimePath = argc >= 4 ? new std::filesystem::path(argv[3]) : nullptr;
            const std::filesystem::path outputDirectory = argc >= 5 ? std::filesystem::path(argv[4]) : (std::filesystem::current_path() / "demo_bundle");
            const std::filesystem::path* pluginDirectory = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
            const int code = BundleProject(argv[2], runtimePath, outputDirectory, executableDirectory, pluginDirectory);
            delete runtimePath;
            delete pluginDirectory;
            return code;
        }
        if (command == "run-bundle" && argc >= 3)
        {
            return RunBundle(argv[2]);
        }
        if (command == "run-batch" && argc >= 3)
        {
            return RunBatch(argv[2]);
        }
        if (command == "report-batch" && argc >= 3)
        {
            const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "batch_report.html");
            return ReportBatch(argv[2], outputPath);
        }
        if (command == "project-tree" && argc >= 3)
        {
            return PrintProjectTree(argv[2]);
        }
        if (command == "record-job-session" && argc >= 3)
        {
            const std::filesystem::path outputDirectory = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "apex_session");
            const std::filesystem::path* configPath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
            const std::filesystem::path* pluginDirectory = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
            const int code = RecordJobSession(argv[2], outputDirectory, configPath, pluginDirectory);
            delete configPath;
            delete pluginDirectory;
            return code;
        }
        if (command == "dashboard-project" && argc >= 3)
        {
            const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "workstation_dashboard.html");
            const std::filesystem::path* configPath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
            const std::filesystem::path* pluginDirectory = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
            const int code = DashboardProject(argv[2], outputPath, configPath, pluginDirectory);
            delete configPath;
            delete pluginDirectory;
            return code;
        }
        if (command == "dashboard-batch" && argc >= 3)
        {
            const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "batch_dashboard.html");
            return DashboardBatch(argv[2], outputPath);
        }

if (command == "export-demo-recipe")
{
    const std::filesystem::path outputPath = argc >= 3 ? std::filesystem::path(argv[2]) : (std::filesystem::current_path() / "painting_standard.arsrecipe");
    return ExportDemoRecipe(outputPath);
}
if (command == "export-demo-patch")
{
    const std::filesystem::path outputPath = argc >= 3 ? std::filesystem::path(argv[2]) : (std::filesystem::current_path() / "shift_fixture.arspatch");
    return ExportDemoPatch(outputPath);
}
if (command == "apply-recipe" && argc >= 4)
{
    const std::filesystem::path outputPath = argc >= 5 ? std::filesystem::path(argv[4]) : (std::filesystem::current_path() / "recipe_applied.arsproject");
    return ApplyRecipeToProject(argv[2], argv[3], outputPath);
}
if (command == "apply-project-patch" && argc >= 4)
{
    const std::filesystem::path outputPath = argc >= 5 ? std::filesystem::path(argv[4]) : (std::filesystem::current_path() / "patched_project.arsproject");
    return ApplyPatchToProject(argv[2], argv[3], outputPath);
}
if (command == "gate-project" && argc >= 3)
{
    const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "release_gate.html");
    const std::filesystem::path* configPath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
    const std::filesystem::path* pluginDirectory = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
    const int code = GateProject(argv[2], outputPath, configPath, pluginDirectory);
    delete configPath;
    delete pluginDirectory;
    return code;
}
if (command == "diff-project" && argc >= 4)
{
    const std::filesystem::path* outputPath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
    const int code = DiffProject(argv[2], argv[3], outputPath);
    delete outputPath;
    return code;
}

if (command == "export-demo-template")
{
    const std::filesystem::path outputPath = argc >= 3 ? std::filesystem::path(argv[2]) : (std::filesystem::current_path() / "paint_cell_template.arstemplate");
    return ExportDemoTemplate(outputPath);
}
if (command == "instantiate-template" && argc >= 3)
{
    const std::filesystem::path outputPath = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "instantiated_project.arsproject");
    const std::string* projectName = argc >= 5 ? new std::string(argv[4]) : nullptr;
    const int code = InstantiateTemplateProject(argv[2], outputPath, projectName);
    delete projectName;
    return code;
}
if (command == "regress-project" && argc >= 4)
{
    const std::filesystem::path outputPath = argc >= 5 ? std::filesystem::path(argv[4]) : (std::filesystem::current_path() / "project_regression.html");
    const std::filesystem::path* configPath = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
    const std::filesystem::path* pluginDirectory = argc >= 7 ? new std::filesystem::path(argv[6]) : nullptr;
    const int code = RegressProject(argv[2], argv[3], outputPath, configPath, pluginDirectory);
    delete configPath;
    delete pluginDirectory;
    return code;
}
if (command == "create-delivery-dossier" && argc >= 3)
{
    const std::filesystem::path outputDirectory = argc >= 4 ? std::filesystem::path(argv[3]) : (std::filesystem::current_path() / "delivery_dossier");
    const std::filesystem::path* configPath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
    const std::filesystem::path* pluginDirectory = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
    const int code = CreateDeliveryDossier(argv[2], outputDirectory, configPath, pluginDirectory);
    delete configPath;
    delete pluginDirectory;
    return code;
}
if (command == "export-approval-template")
{
    return ExportApprovalTemplate(argc >= 3 ? argv[2] : std::filesystem::path("demo_approval.arsapproval"));
}
if (command == "sign-project" && argc >= 6)
{
    const std::string decision = argc >= 7 ? argv[6] : std::string("approved");
    const std::string notes = argc >= 8 ? argv[7] : std::string("Signed from CLI workflow.");
    return SignProjectApproval(argv[2], argv[3], argv[4], argv[5], decision, notes);
}
if (command == "verify-approval" && argc >= 4)
{
    return VerifyProjectApproval(argv[2], argv[3], argc >= 5 ? argv[4] : std::filesystem::path("approval_report.html"));
}
if (command == "create-project-snapshot" && argc >= 3)
{
    const std::filesystem::path outputDirectory = argc >= 4 ? argv[3] : std::filesystem::path("project_snapshot");
    const std::filesystem::path* runtimePath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
    const std::filesystem::path* approvalPath = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
    const int code = CreateProjectSnapshot(argv[2], outputDirectory, runtimePath, approvalPath);
    delete runtimePath;
    delete approvalPath;
    return code;
}
if (command == "restore-project-snapshot" && argc >= 3)
{
    const std::filesystem::path outputProject = argc >= 4 ? argv[3] : std::filesystem::path("restored_project.arsproject");
    const std::filesystem::path* outputRuntime = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
    const std::filesystem::path* outputApproval = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
    const int code = RestoreProjectSnapshot(argv[2], outputProject, outputRuntime, outputApproval);
    delete outputRuntime;
    delete outputApproval;
    return code;
}
if (command == "export-integration-package" && argc >= 3)
{
    const std::filesystem::path outputDirectory = argc >= 4 ? argv[3] : std::filesystem::path("integration_package");
    const std::filesystem::path* runtimePath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
    const std::filesystem::path* pluginDirectory = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
    const std::filesystem::path* approvalPath = argc >= 7 ? new std::filesystem::path(argv[6]) : nullptr;
    const int code = ExportIntegrationPackage(argv[2], outputDirectory, runtimePath, pluginDirectory, approvalPath);
    delete runtimePath;
    delete pluginDirectory;
    delete approvalPath;
    return code;
}

if (command == "export-demo-flow")
{
    const std::filesystem::path outputPath = argc >= 3 ? argv[2] : std::filesystem::path("demo_workflow_v11.arsflow");
    return ExportDemoFlow(outputPath, executableDirectory);
}
if (command == "run-flow" && argc >= 3)
{
    return RunFlow(argv[2]);
}
if (command == "export-demo-sweep")
{
    const std::filesystem::path outputPath = argc >= 3 ? argv[2] : std::filesystem::path("demo_parameter_sweep_v11.arssweep");
    return ExportDemoSweep(outputPath);
}
if (command == "run-sweep" && argc >= 3)
{
    return RunSweep(argv[2]);
}
if (command == "audit-project" && argc >= 3)
{
    const std::filesystem::path outputDirectory = argc >= 4 ? argv[3] : std::filesystem::path("project_audit_v11");
    const std::filesystem::path* runtimePath = argc >= 5 ? new std::filesystem::path(argv[4]) : nullptr;
    const std::filesystem::path* pluginDirectory = argc >= 6 ? new std::filesystem::path(argv[5]) : nullptr;
    const std::filesystem::path* approvalPath = argc >= 7 ? new std::filesystem::path(argv[6]) : nullptr;
    const int code = AuditProject(argv[2], outputDirectory, runtimePath, pluginDirectory, approvalPath);
    delete runtimePath;
    delete pluginDirectory;
    delete approvalPath;
    return code;
}
if (command == "export-ros2-workspace" && argc >= 3)
{
    const std::filesystem::path outputDirectory = argc >= 4 ? argv[3] : std::filesystem::current_path();
    return ExportRos2Workspace(argv[2], outputDirectory);
}

        PrintUsage();
        return 1;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }
}
