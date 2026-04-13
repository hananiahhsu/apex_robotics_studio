#include "TestFramework.h"

#include "apex/governance/ProjectApproval.h"

#include <filesystem>

namespace
{
    apex::io::StudioProject BuildApprovalTestProject()
    {
        apex::io::StudioProject project;
        project.SchemaVersion = 3;
        project.ProjectName = "Approval Test Project";
        project.RobotName = "Robot";
        constexpr double limit = 3.14;
        project.Joints.push_back({"J1", 0.4, -limit, limit});
        project.Motion.Start.JointAnglesRad = {0.0};
        project.Motion.Goal.JointAnglesRad = {1.0};
        project.Job.Name = "Job";
        project.Job.LinkSafetyRadius = 0.08;
        project.Job.Waypoints.push_back({"A", {{0.0}}});
        project.Job.Waypoints.push_back({"B", {{1.0}}});
        project.Job.Segments.push_back({"Move", "A", "B", 8, 2.0, "process"});
        return project;
    }
}

ARS_TEST(TestProjectApprovalWorkflowVerify)
{
    const auto project = BuildApprovalTestProject();
    apex::governance::ProjectApprovalWorkflow workflow;
    auto document = workflow.CreateTemplate(project);

    workflow.Sign(project, document, "robotics_lead", "alice", apex::governance::ApprovalDecision::Approved, "Looks good.");
    workflow.Sign(project, document, "process_owner", "bob", apex::governance::ApprovalDecision::Approved, "Approved process.");
    workflow.Sign(project, document, "qa", "carol", apex::governance::ApprovalDecision::Approved, "QA pass.");

    const auto verification = workflow.Verify(project, document);
    apex::tests::Require(verification.OverallDecision == apex::governance::ApprovalDecision::Approved, "All required roles should approve the current fingerprint.");
    apex::tests::Require(verification.RoleStatuses.size() == 3, "Three role statuses should be reported.");
}

ARS_TEST(TestProjectApprovalRoundTrip)
{
    const auto project = BuildApprovalTestProject();
    apex::governance::ProjectApprovalWorkflow workflow;
    auto document = workflow.CreateTemplate(project);
    workflow.Sign(project, document, "robotics_lead", "alice", apex::governance::ApprovalDecision::Approved, "Signed.");

    const auto filePath = std::filesystem::temp_directory_path() / "apex_project_approval_test.arsapproval";
    apex::governance::ProjectApprovalSerializer().SaveToFile(document, filePath);
    const auto loaded = apex::governance::ProjectApprovalSerializer().LoadFromFile(filePath);

    apex::tests::Require(loaded.RequiredRoles.size() == document.RequiredRoles.size(), "Required roles should round-trip.");
    apex::tests::Require(loaded.Entries.size() == 1, "One signature entry should round-trip.");
    apex::tests::Require(loaded.Entries.front().Signer == "alice", "Signer should round-trip.");

    std::error_code ec;
    std::filesystem::remove(filePath, ec);
}
