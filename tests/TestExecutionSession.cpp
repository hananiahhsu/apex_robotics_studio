#include "TestFramework.h"

#include "apex/session/ExecutionSession.h"

#include <filesystem>
#include <fstream>

ARS_TEST(TestExecutionSessionSavesArtifacts)
{
    const std::filesystem::path outputDirectory = std::filesystem::temp_directory_path() / "apex_session_test_v07";
    std::filesystem::remove_all(outputDirectory);

    apex::session::SessionRecorder recorder;
    recorder.SetTitle("Demo Session");
    recorder.SetProjectName("Project");
    recorder.SetJobName("Job");
    recorder.SetMetrics(1.2, 3.4, 2);
    recorder.AddWarning("Review collision margin.");
    recorder.AddEvent("job", "info", "Started execution.");
    recorder.AddArtifact("report", "report.html", "Generated report");
    recorder.MarkStatus(apex::session::SessionStatus::Warning);
    recorder.SaveToDirectory(outputDirectory);

    apex::tests::Require(std::filesystem::exists(outputDirectory / "session_summary.json"), "Session JSON should exist.");
    apex::tests::Require(std::filesystem::exists(outputDirectory / "session_events.jsonl"), "Session event JSONL should exist.");
    apex::tests::Require(std::filesystem::exists(outputDirectory / "session_summary.html"), "Session HTML should exist.");

    std::ifstream jsonStream(outputDirectory / "session_summary.json");
    std::string jsonText((std::istreambuf_iterator<char>(jsonStream)), std::istreambuf_iterator<char>());
    apex::tests::Require(jsonText.find("Demo Session") != std::string::npos, "Session JSON should contain title.");
    std::filesystem::remove_all(outputDirectory);
}
