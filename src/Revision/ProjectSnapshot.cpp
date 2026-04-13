#include "apex/revision/ProjectSnapshot.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace apex::revision
{
    namespace
    {
        std::string NowUtcString()
        {
            const auto now = std::chrono::system_clock::now();
            const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
            std::tm timeInfo{};
        #if defined(_WIN32)
            gmtime_s(&timeInfo, &nowTime);
        #else
            gmtime_r(&nowTime, &timeInfo);
        #endif
            std::ostringstream builder;
            builder << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%SZ");
            return builder.str();
        }

        std::string EscapeJson(const std::string& value)
        {
            std::string output;
            output.reserve(value.size());
            for (const char ch : value)
            {
                switch (ch)
                {
                case '\\': output += "\\\\"; break;
                case '"': output += "\\\""; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default: output += ch; break;
                }
            }
            return output;
        }
    }

    ProjectSnapshotManifest ProjectSnapshotManager::CreateSnapshot(
        const std::filesystem::path& projectFilePath,
        const std::filesystem::path& outputDirectory,
        const std::filesystem::path* runtimeConfigPath,
        const std::filesystem::path* approvalPath) const
    {
        if (!std::filesystem::exists(projectFilePath))
        {
            throw std::runtime_error("Project file does not exist: " + projectFilePath.string());
        }
        std::filesystem::create_directories(outputDirectory / "project");
        std::filesystem::create_directories(outputDirectory / "config");
        std::filesystem::create_directories(outputDirectory / "governance");

        ProjectSnapshotManifest manifest;
        manifest.SnapshotId = "snapshot-" + NowUtcString();
        manifest.ProjectName = projectFilePath.stem().string();
        manifest.ProjectFingerprint = std::to_string(std::filesystem::file_size(projectFilePath));
        manifest.CreatedAtUtc = NowUtcString();
        manifest.SourceProjectPath = projectFilePath;

        const auto projectTarget = outputDirectory / "project" / projectFilePath.filename();
        std::filesystem::copy_file(projectFilePath, projectTarget, std::filesystem::copy_options::overwrite_existing);
        manifest.Artifacts.push_back({"project", std::filesystem::path("project") / projectFilePath.filename(), "Project snapshot payload"});

        if (runtimeConfigPath != nullptr && std::filesystem::exists(*runtimeConfigPath))
        {
            const auto runtimeTarget = outputDirectory / "config" / runtimeConfigPath->filename();
            std::filesystem::copy_file(*runtimeConfigPath, runtimeTarget, std::filesystem::copy_options::overwrite_existing);
            manifest.Artifacts.push_back({"runtime", std::filesystem::path("config") / runtimeConfigPath->filename(), "Runtime configuration snapshot"});
        }
        if (approvalPath != nullptr && std::filesystem::exists(*approvalPath))
        {
            const auto approvalTarget = outputDirectory / "governance" / approvalPath->filename();
            std::filesystem::copy_file(*approvalPath, approvalTarget, std::filesystem::copy_options::overwrite_existing);
            manifest.Artifacts.push_back({"approval", std::filesystem::path("governance") / approvalPath->filename(), "Approval workflow snapshot"});
        }

        SaveManifestJson(manifest, outputDirectory / "snapshot_manifest.json");
        SaveIndexHtml(manifest, outputDirectory / "index.html");
        return manifest;
    }

    void ProjectSnapshotManager::RestoreSnapshot(
        const std::filesystem::path& snapshotDirectory,
        const std::filesystem::path& outputProjectPath,
        const std::filesystem::path* outputRuntimePath,
        const std::filesystem::path* outputApprovalPath) const
    {
        const auto projectDir = snapshotDirectory / "project";
        if (!std::filesystem::exists(projectDir))
        {
            throw std::runtime_error("Snapshot does not contain project directory: " + snapshotDirectory.string());
        }
        std::filesystem::path sourceProjectPath;
        for (const auto& entry : std::filesystem::directory_iterator(projectDir))
        {
            if (entry.is_regular_file())
            {
                sourceProjectPath = entry.path();
                break;
            }
        }
        if (sourceProjectPath.empty())
        {
            throw std::runtime_error("Snapshot project payload not found: " + snapshotDirectory.string());
        }
        if (outputProjectPath.has_parent_path())
        {
            std::filesystem::create_directories(outputProjectPath.parent_path());
        }
        std::filesystem::copy_file(sourceProjectPath, outputProjectPath, std::filesystem::copy_options::overwrite_existing);

        if (outputRuntimePath != nullptr)
        {
            const auto configDir = snapshotDirectory / "config";
            if (std::filesystem::exists(configDir))
            {
                for (const auto& entry : std::filesystem::directory_iterator(configDir))
                {
                    if (entry.is_regular_file())
                    {
                        if (outputRuntimePath->has_parent_path())
                        {
                            std::filesystem::create_directories(outputRuntimePath->parent_path());
                        }
                        std::filesystem::copy_file(entry.path(), *outputRuntimePath, std::filesystem::copy_options::overwrite_existing);
                        break;
                    }
                }
            }
        }

        if (outputApprovalPath != nullptr)
        {
            const auto governanceDir = snapshotDirectory / "governance";
            if (std::filesystem::exists(governanceDir))
            {
                for (const auto& entry : std::filesystem::directory_iterator(governanceDir))
                {
                    if (entry.is_regular_file())
                    {
                        if (outputApprovalPath->has_parent_path())
                        {
                            std::filesystem::create_directories(outputApprovalPath->parent_path());
                        }
                        std::filesystem::copy_file(entry.path(), *outputApprovalPath, std::filesystem::copy_options::overwrite_existing);
                        break;
                    }
                }
            }
        }
    }

    void ProjectSnapshotManager::SaveManifestJson(const ProjectSnapshotManifest& manifest, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write snapshot manifest: " + filePath.string());
        }
        stream << "{\n"
               << "  \"schema_version\": " << manifest.SchemaVersion << ",\n"
               << "  \"snapshot_id\": \"" << EscapeJson(manifest.SnapshotId) << "\",\n"
               << "  \"project_name\": \"" << EscapeJson(manifest.ProjectName) << "\",\n"
               << "  \"project_fingerprint\": \"" << EscapeJson(manifest.ProjectFingerprint) << "\",\n"
               << "  \"created_at_utc\": \"" << EscapeJson(manifest.CreatedAtUtc) << "\",\n"
               << "  \"source_project_path\": \"" << EscapeJson(manifest.SourceProjectPath.generic_string()) << "\",\n"
               << "  \"artifacts\": [\n";
        for (std::size_t index = 0; index < manifest.Artifacts.size(); ++index)
        {
            const auto& artifact = manifest.Artifacts[index];
            stream << "    {\"type\": \"" << EscapeJson(artifact.Type)
                   << "\", \"relative_path\": \"" << EscapeJson(artifact.RelativePath.generic_string())
                   << "\", \"description\": \"" << EscapeJson(artifact.Description) << "\"}";
            if (index + 1 < manifest.Artifacts.size())
            {
                stream << ',';
            }
            stream << '\n';
        }
        stream << "  ]\n}";
    }

    std::string ProjectSnapshotManager::BuildIndexHtml(const ProjectSnapshotManifest& manifest) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Project Snapshot</title>"
             << "<style>body{font-family:Arial,sans-serif;margin:24px;}table{border-collapse:collapse;width:100%;}"
             << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}th{background:#f5f5f5;}</style></head><body>";
        html << "<h1>Project Snapshot</h1><p><strong>Snapshot:</strong> " << manifest.SnapshotId
             << "<br><strong>Project:</strong> " << manifest.ProjectName
             << "<br><strong>Fingerprint:</strong> " << manifest.ProjectFingerprint
             << "<br><strong>Created:</strong> " << manifest.CreatedAtUtc << "</p>";
        html << "<table><thead><tr><th>Type</th><th>Artifact</th><th>Description</th></tr></thead><tbody>";
        for (const auto& artifact : manifest.Artifacts)
        {
            html << "<tr><td>" << artifact.Type << "</td><td><a href=\"" << artifact.RelativePath.generic_string() << "\">"
                 << artifact.RelativePath.generic_string() << "</a></td><td>" << artifact.Description << "</td></tr>";
        }
        html << "</tbody></table></body></html>";
        return html.str();
    }

    void ProjectSnapshotManager::SaveIndexHtml(const ProjectSnapshotManifest& manifest, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write snapshot index: " + filePath.string());
        }
        stream << BuildIndexHtml(manifest);
    }
}
