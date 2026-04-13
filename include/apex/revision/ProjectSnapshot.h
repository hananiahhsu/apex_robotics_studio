#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace apex::revision
{
    struct SnapshotArtifact
    {
        std::string Type;
        std::filesystem::path RelativePath;
        std::string Description;
    };

    struct ProjectSnapshotManifest
    {
        int SchemaVersion = 1;
        std::string SnapshotId;
        std::string ProjectName;
        std::string ProjectFingerprint;
        std::string CreatedAtUtc;
        std::filesystem::path SourceProjectPath;
        std::vector<SnapshotArtifact> Artifacts;
    };

    class ProjectSnapshotManager
    {
    public:
        [[nodiscard]] ProjectSnapshotManifest CreateSnapshot(
            const std::filesystem::path& projectFilePath,
            const std::filesystem::path& outputDirectory,
            const std::filesystem::path* runtimeConfigPath = nullptr,
            const std::filesystem::path* approvalPath = nullptr) const;

        void RestoreSnapshot(
            const std::filesystem::path& snapshotDirectory,
            const std::filesystem::path& outputProjectPath,
            const std::filesystem::path* outputRuntimePath = nullptr,
            const std::filesystem::path* outputApprovalPath = nullptr) const;

        void SaveManifestJson(const ProjectSnapshotManifest& manifest, const std::filesystem::path& filePath) const;
        [[nodiscard]] std::string BuildIndexHtml(const ProjectSnapshotManifest& manifest) const;
        void SaveIndexHtml(const ProjectSnapshotManifest& manifest, const std::filesystem::path& filePath) const;
    };
}
