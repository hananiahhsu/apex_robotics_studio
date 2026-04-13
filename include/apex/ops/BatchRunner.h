#pragma once

#include "apex/extension/ProjectAuditPlugin.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::ops
{
    struct BatchEntry
    {
        std::string Name;
        std::filesystem::path ProjectPath;
        std::filesystem::path RuntimeConfigPath;
        std::filesystem::path PluginDirectoryPath;
    };

    struct BatchManifest
    {
        int SchemaVersion = 1;
        std::string Name;
        std::vector<BatchEntry> Entries;
    };

    struct BatchEntryResult
    {
        BatchEntry Entry;
        bool Success = false;
        bool CollisionFree = false;
        double TotalDurationSeconds = 0.0;
        double TotalPathLengthMeters = 0.0;
        std::size_t FindingCount = 0;
        std::vector<std::string> Warnings;
    };

    struct BatchRunResult
    {
        std::string Name;
        std::vector<BatchEntryResult> Entries;
        std::size_t SuccessCount = 0;
        std::size_t FailureCount = 0;
    };

    class BatchManifestLoader
    {
    public:
        [[nodiscard]] BatchManifest LoadFromFile(const std::filesystem::path& filePath) const;
        void SaveToFile(const BatchManifest& manifest, const std::filesystem::path& filePath) const;
    };

    class BatchRunner
    {
    public:
        [[nodiscard]] BatchRunResult Run(const BatchManifest& manifest) const;
        [[nodiscard]] std::string BuildHtmlReport(const BatchRunResult& result) const;
        void SaveHtmlReport(const BatchRunResult& result, const std::filesystem::path& filePath) const;
    };
}
