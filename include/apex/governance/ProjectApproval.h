#pragma once

#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::governance
{
    enum class ApprovalDecision
    {
        Pending,
        Approved,
        Rejected
    };

    struct ApprovalEntry
    {
        std::string Role;
        std::string Signer;
        ApprovalDecision Decision = ApprovalDecision::Pending;
        std::string Notes;
        std::string TimestampUtc;
        std::string ProjectFingerprint;
    };

    struct ProjectApprovalDocument
    {
        int SchemaVersion = 1;
        std::string Title;
        std::string ProjectName;
        std::vector<std::string> RequiredRoles;
        std::vector<ApprovalEntry> Entries;
    };

    struct RoleApprovalStatus
    {
        std::string Role;
        bool IsSatisfied = false;
        bool HasCurrentApproval = false;
        bool HasCurrentRejection = false;
        std::string LatestSigner;
        std::string LatestTimestampUtc;
        std::string LatestNotes;
    };

    struct ProjectApprovalVerification
    {
        ApprovalDecision OverallDecision = ApprovalDecision::Pending;
        std::string ProjectFingerprint;
        std::vector<RoleApprovalStatus> RoleStatuses;
        std::vector<std::string> Notes;
    };

    class ProjectFingerprintBuilder
    {
    public:
        [[nodiscard]] std::string Build(const apex::io::StudioProject& project) const;
    };

    class ProjectApprovalSerializer
    {
    public:
        [[nodiscard]] ProjectApprovalDocument LoadFromFile(const std::filesystem::path& filePath) const;
        void SaveToFile(const ProjectApprovalDocument& document, const std::filesystem::path& filePath) const;
    };

    class ProjectApprovalWorkflow
    {
    public:
        [[nodiscard]] ProjectApprovalDocument CreateTemplate(const apex::io::StudioProject& project) const;
        void Sign(
            const apex::io::StudioProject& project,
            ProjectApprovalDocument& document,
            const std::string& role,
            const std::string& signer,
            ApprovalDecision decision,
            const std::string& notes) const;

        [[nodiscard]] ProjectApprovalVerification Verify(
            const apex::io::StudioProject& project,
            const ProjectApprovalDocument& document) const;

        [[nodiscard]] std::string BuildHtmlReport(
            const apex::io::StudioProject& project,
            const ProjectApprovalDocument& document,
            const ProjectApprovalVerification& verification) const;

        void SaveHtmlReport(
            const apex::io::StudioProject& project,
            const ProjectApprovalDocument& document,
            const ProjectApprovalVerification& verification,
            const std::filesystem::path& filePath) const;
    };

    [[nodiscard]] std::string ToString(ApprovalDecision decision) noexcept;
    [[nodiscard]] ApprovalDecision ApprovalDecisionFromString(const std::string& text);
}
