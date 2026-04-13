#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace apex::trace
{
    struct AuditArtifact
    {
        std::string Type;
        std::filesystem::path RelativePath;
        std::string Description;
    };

    struct AuditEvent
    {
        int Sequence = 0;
        std::string Category;
        std::string Level;
        std::string Action;
        std::string Message;
    };

    struct AuditTrailSummary
    {
        std::string Title;
        std::string ProjectName;
        std::string ProjectFingerprint;
        std::string OverallStatus = "success";
        std::vector<AuditArtifact> Artifacts;
        std::vector<AuditEvent> Events;
    };

    class AuditTrailRecorder
    {
    public:
        void SetTitle(std::string title);
        void SetProjectName(std::string projectName);
        void SetProjectFingerprint(std::string fingerprint);
        void SetOverallStatus(std::string overallStatus);
        void AddArtifact(std::string type, std::filesystem::path relativePath, std::string description);
        void AddEvent(std::string category, std::string level, std::string action, std::string message);
        [[nodiscard]] const AuditTrailSummary& GetSummary() const noexcept;
        void SaveToDirectory(const std::filesystem::path& directory) const;
        [[nodiscard]] std::string BuildHtmlIndex() const;
    private:
        AuditTrailSummary m_summary;
    };
}
