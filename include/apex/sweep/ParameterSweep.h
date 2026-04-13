#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace apex::sweep
{
    struct SweepDefinition
    {
        int SchemaVersion = 1;
        std::string Name;
        std::filesystem::path BaseProjectPath;
        std::filesystem::path RuntimePath;
        std::filesystem::path PluginDirectory;
        std::filesystem::path OutputDirectory;
        std::vector<std::size_t> SampleCounts;
        std::vector<double> DurationScales;
        std::vector<double> SafetyRadii;
    };

    struct SweepVariantSummary
    {
        std::string VariantName;
        std::size_t SampleCount = 0;
        double DurationScale = 1.0;
        double SafetyRadius = 0.0;
        std::string GateStatus;
        bool CollisionFree = true;
        std::size_t FindingCount = 0;
        double TotalDurationSeconds = 0.0;
        double TotalPathLengthMeters = 0.0;
    };

    class ParameterSweepSerializer
    {
    public:
        [[nodiscard]] SweepDefinition LoadFromFile(const std::filesystem::path& filePath) const;
        void SaveToFile(const SweepDefinition& sweep, const std::filesystem::path& filePath) const;
    };

    class ParameterSweepReportBuilder
    {
    public:
        void SaveCsv(const std::vector<SweepVariantSummary>& variants, const std::filesystem::path& filePath) const;
        [[nodiscard]] std::string BuildHtml(const SweepDefinition& sweep, const std::vector<SweepVariantSummary>& variants) const;
        void SaveHtml(const SweepDefinition& sweep, const std::vector<SweepVariantSummary>& variants, const std::filesystem::path& filePath) const;
    };
}
