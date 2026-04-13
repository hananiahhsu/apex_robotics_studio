#include "TestFramework.h"

#include "apex/trace/AuditTrail.h"

#include <filesystem>
#include <fstream>

ARS_TEST(TestAuditTrailSave)
{
    const auto dir = std::filesystem::temp_directory_path() / "apex_audit_trail_test_v11";
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);

    apex::trace::AuditTrailRecorder recorder;
    recorder.SetTitle("Audit Test");
    recorder.SetProjectName("Project A");
    recorder.SetProjectFingerprint("fingerprint-123");
    recorder.AddArtifact("report", "reports/test.html", "Report");
    recorder.AddEvent("audit", "info", "load", "Loaded project");
    recorder.SaveToDirectory(dir);

    apex::tests::Require(std::filesystem::exists(dir / "audit_summary.json"), "Audit summary JSON should exist.");
    apex::tests::Require(std::filesystem::exists(dir / "audit_events.jsonl"), "Audit JSONL should exist.");
    apex::tests::Require(std::filesystem::exists(dir / "audit_index.html"), "Audit HTML should exist.");

    std::filesystem::remove_all(dir, ec);
}
