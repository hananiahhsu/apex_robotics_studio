#pragma once

#include <filesystem>
#include <string>

namespace apex::bundle
{
    struct ProjectBundleManifest
    {
        int SchemaVersion = 1;
        std::string BundleName;
        std::filesystem::path ProjectRelativePath = std::filesystem::path("project") / "workcell.arsproject";
        std::filesystem::path RuntimeRelativePath = std::filesystem::path("config") / "runtime.ini";
        std::filesystem::path PluginDirectoryRelativePath = "plugins";
        std::filesystem::path ReportsDirectoryRelativePath = "reports";
    };

    class ProjectBundleManager
    {
    public:
        void CreateBundle(
            const std::filesystem::path& projectFilePath,
            const std::filesystem::path& runtimeConfigPath,
            const std::filesystem::path& outputDirectory,
            const std::filesystem::path* pluginDirectory = nullptr) const;

        [[nodiscard]] ProjectBundleManifest LoadManifest(const std::filesystem::path& bundleDirectory) const;
        void SaveManifest(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const;

        [[nodiscard]] std::filesystem::path ResolveProjectPath(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const;
        [[nodiscard]] std::filesystem::path ResolveRuntimePath(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const;
        [[nodiscard]] std::filesystem::path ResolvePluginDirectory(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const;
        [[nodiscard]] std::filesystem::path ResolveReportsDirectory(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const;
    };
}
