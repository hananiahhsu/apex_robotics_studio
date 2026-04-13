#include "TestFramework.h"

#include "apex/revision/ProjectSnapshot.h"

#include <filesystem>
#include <fstream>

ARS_TEST(TestProjectSnapshotCreateAndRestore)
{
    const auto tempRoot = std::filesystem::temp_directory_path() / "apex_snapshot_test";
    const auto projectPath = tempRoot / "input.arsproject";
    const auto snapshotDir = tempRoot / "snapshot";
    const auto restoredProjectPath = tempRoot / "restored" / "output.arsproject";
    const auto runtimePath = tempRoot / "runtime.ini";
    const auto approvalPath = tempRoot / "approval.arsapproval";

    std::filesystem::create_directories(tempRoot);
    {
        std::ofstream project(projectPath);
        project << "[project]\nschema_version=1\nproject_name=Snapshot\nrobot_name=Robot\n";
        std::ofstream runtime(runtimePath);
        runtime << "log_level=info\n";
        std::ofstream approval(approvalPath);
        approval << "[approval]\nschema_version=1\ntitle=Approval\nproject_name=Snapshot\n";
    }

    apex::revision::ProjectSnapshotManager manager;
    const auto manifest = manager.CreateSnapshot(projectPath, snapshotDir, &runtimePath, &approvalPath);
    apex::tests::Require(!manifest.SnapshotId.empty(), "Snapshot id should be generated.");
    apex::tests::Require(std::filesystem::exists(snapshotDir / "snapshot_manifest.json"), "Snapshot manifest should exist.");

    const auto restoredRuntimePath = tempRoot / "restored" / "runtime.ini";
    const auto restoredApprovalPath = tempRoot / "restored" / "approval.arsapproval";
    manager.RestoreSnapshot(snapshotDir, restoredProjectPath, &restoredRuntimePath, &restoredApprovalPath);

    apex::tests::Require(std::filesystem::exists(restoredProjectPath), "Project file should be restored.");
    apex::tests::Require(std::filesystem::exists(restoredRuntimePath), "Runtime file should be restored.");
    apex::tests::Require(std::filesystem::exists(restoredApprovalPath), "Approval file should be restored.");

    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}
