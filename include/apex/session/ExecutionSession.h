#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace apex::session
{
    enum class SessionStatus
    {
        Success,
        Warning,
        Failure
    };

    struct SessionEvent
    {
        int Sequence = 0;
        std::string Category;
        std::string Level;
        std::string Message;
    };

    struct SessionArtifact
    {
        std::string Type;
        std::filesystem::path Path;
        std::string Description;
    };

    struct ExecutionSessionSummary
    {
        std::string Title;
        std::string ProjectName;
        std::string JobName;
        SessionStatus Status = SessionStatus::Success;
        double TotalDurationSeconds = 0.0;
        double TotalPathLengthMeters = 0.0;
        std::size_t FindingCount = 0;
        std::vector<std::string> Warnings;
        std::vector<SessionEvent> Events;
        std::vector<SessionArtifact> Artifacts;
    };

    class SessionRecorder
    {
    public:
        void SetTitle(std::string title);
        void SetProjectName(std::string projectName);
        void SetJobName(std::string jobName);
        void SetMetrics(double totalDurationSeconds, double totalPathLengthMeters, std::size_t findingCount);
        void AddWarning(const std::string& warning);
        void AddArtifact(std::string type, std::filesystem::path path, std::string description);
        void AddEvent(std::string category, std::string level, std::string message);
        void MarkStatus(SessionStatus status) noexcept;

        [[nodiscard]] const ExecutionSessionSummary& GetSummary() const noexcept;
        void SaveToDirectory(const std::filesystem::path& directory) const;
        [[nodiscard]] std::string BuildSummaryHtml() const;
    private:
        ExecutionSessionSummary m_summary;
    };

    [[nodiscard]] const char* ToString(SessionStatus status) noexcept;
}
