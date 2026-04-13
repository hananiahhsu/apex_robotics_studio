#include "TestFramework.h"
#include "apex/io/StudioProjectSerializer.h"
#include "apex/ops/BatchRunner.h"
#include "apex/platform/RuntimeConfig.h"

#include <filesystem>

namespace
{
    apex::io::StudioProject BuildBatchProject()
    {
        apex::io::StudioProject project;
        project.SchemaVersion = 2;
        project.ProjectName = "BatchTest";
        project.RobotName = "BatchBot";
        constexpr double jointLimit = 3.14159265358979323846;
        project.Joints.push_back({"J1", 0.5, -jointLimit, jointLimit});
        project.Joints.push_back({"J2", 0.4, -jointLimit, jointLimit});
        project.Motion.Start.JointAnglesRad = {0.0, 0.0};
        project.Motion.Goal.JointAnglesRad = {0.1, -0.1};
        project.Motion.SampleCount = 8;
        project.Motion.DurationSeconds = 1.5;
        project.Motion.LinkSafetyRadius = 0.05;
        project.Job.Name = "BatchDemoJob";
        project.Job.LinkSafetyRadius = 0.05;
        project.Job.Waypoints.push_back({"Home", {{0.0, 0.0}}});
        project.Job.Waypoints.push_back({"Work", {{0.1, -0.1}}});
        project.Job.Segments.push_back({"Move", "Home", "Work", 8, 1.5, "demo"});
        return project;
    }
}

ARS_TEST(TestBatchRunnerRoundTrip)
{
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "apex_batch_test";
    std::error_code errorCode;
    std::filesystem::remove_all(tempRoot, errorCode);
    std::filesystem::create_directories(tempRoot);

    const auto projectPath = tempRoot / "batch_test.arsproject";
    const auto runtimePath = tempRoot / "runtime.ini";
    const auto batchPath = tempRoot / "demo.arsbatch";

    const apex::io::StudioProjectSerializer serializer;
    serializer.SaveToFile(BuildBatchProject(), projectPath);
    const apex::platform::RuntimeConfigLoader runtimeLoader;
    runtimeLoader.SaveDefaultFile(runtimePath);

    apex::ops::BatchManifest manifest;
    manifest.Name = "Batch Validation";
    manifest.Entries.push_back({"entry_01", projectPath, runtimePath, {}});

    const apex::ops::BatchManifestLoader loader;
    loader.SaveToFile(manifest, batchPath);
    const auto loaded = loader.LoadFromFile(batchPath);
    apex::tests::Require(loaded.Entries.size() == 1, "Loaded batch should have one entry");

    const apex::ops::BatchRunner runner;
    const auto result = runner.Run(loaded);
    apex::tests::Require(result.SuccessCount == 1, "Batch run should succeed");
    apex::tests::Require(result.Entries.size() == 1, "Batch result should have one entry");

    const auto html = runner.BuildHtmlReport(result);
    apex::tests::Require(html.find("Batch Validation") != std::string::npos, "Batch HTML report should contain batch name");

    std::filesystem::remove_all(tempRoot, errorCode);
}
