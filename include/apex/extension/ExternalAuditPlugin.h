#pragma once

#include "apex/extension/ProjectAuditPlugin.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::extension
{
    struct ExternalAuditPluginManifest
    {
        std::string Name;
        std::filesystem::path ExecutablePath;
        std::string ArgumentsTemplate;
        std::filesystem::path ManifestPath;
    };

    class ExternalAuditPluginRunner
    {
    public:
        [[nodiscard]] std::vector<ExternalAuditPluginManifest> Discover(const std::filesystem::path& pluginDirectory) const;
        [[nodiscard]] std::vector<PluginFinding> Run(
            const std::filesystem::path& projectFilePath,
            const std::filesystem::path& pluginDirectory) const;
    private:
        [[nodiscard]] std::vector<PluginFinding> ParseFindingsFile(
            const std::string& pluginName,
            const std::filesystem::path& findingsFilePath) const;
    };
}
