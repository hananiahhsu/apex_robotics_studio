#include "TestFramework.h"
#include "apex/io/StudioProjectSerializer.h"

#include <filesystem>

ARS_TEST(TestProjectSerializerRoundTrip)
{
    const std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "apex_roundtrip_test.arsproject";

    apex::io::StudioProject project;
    project.SchemaVersion = 2;
    project.ProjectName = "RoundTrip";
    project.RobotName = "Robot";
    project.Joints.push_back({ "J1", 1.0, -1.0, 1.0 });
    project.Joints.push_back({ "J2", 0.5, -0.5, 0.5 });
    project.Obstacles.push_back({ "Box", {{0.0, 0.0, 0.0}, {1.0, 1.0, 1.0}} });
    project.Motion.Start.JointAnglesRad = { 0.1, 0.2 };
    project.Motion.Goal.JointAnglesRad = { 0.3, 0.4 };
    project.Motion.SampleCount = 12;
    project.Motion.DurationSeconds = 5.0;
    project.Motion.LinkSafetyRadius = 0.07;
    project.Job.Name = "RoundTripJob";
    project.Job.LinkSafetyRadius = 0.07;
    project.Job.Waypoints.push_back({"Home", {{0.1, 0.2}}});
    project.Job.Waypoints.push_back({"Target", {{0.3, 0.4}}});
    project.Job.Segments.push_back({"Move", "Home", "Target", 10, 2.0, "test"});

    const apex::io::StudioProjectSerializer serializer;
    serializer.SaveToFile(project, tempPath);
    const apex::io::StudioProject loaded = serializer.LoadFromFile(tempPath);

    apex::tests::Require(loaded.ProjectName == "RoundTrip", "Project name should survive serialization");
    apex::tests::Require(loaded.Joints.size() == 2, "Two joints should survive serialization");
    apex::tests::Require(loaded.Obstacles.size() == 1, "One obstacle should survive serialization");
    apex::tests::Require(loaded.Motion.SampleCount == 12, "Sample count should survive serialization");
    apex::tests::RequireNear(loaded.Motion.DurationSeconds, 5.0, 1e-9, "Duration should survive serialization");
    apex::tests::Require(loaded.Job.Waypoints.size() == 2, "Job waypoints should survive serialization");
    apex::tests::Require(loaded.Job.Segments.size() == 1, "Job segments should survive serialization");

    std::error_code errorCode;
    std::filesystem::remove(tempPath, errorCode);
}
