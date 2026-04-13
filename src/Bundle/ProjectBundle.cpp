#include "apex/bundle/ProjectBundle.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace apex::bundle
{
    namespace
    {
        using KeyValueMap = std::unordered_map<std::string, std::string>;

        std::string Trim(std::string value)
        {
            const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        std::string GetRequired(const KeyValueMap& values, const std::string& key)
        {
            const auto iterator = values.find(key);
            if (iterator == values.end())
            {
                throw std::invalid_argument("Missing key '" + key + "' in [bundle] section.");
            }
            return iterator->second;
        }

        KeyValueMap ParseBundleSection(const std::filesystem::path& filePath)
        {
            std::ifstream stream(filePath);
            if (!stream)
            {
                throw std::runtime_error("Failed to open bundle manifest: " + filePath.string());
            }

            KeyValueMap values;
            std::string line;
            bool inBundleSection = false;
            while (std::getline(stream, line))
            {
                const std::string trimmed = Trim(line);
                if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';')
                {
                    continue;
                }
                if (trimmed.front() == '[' && trimmed.back() == ']')
                {
                    inBundleSection = trimmed.substr(1, trimmed.size() - 2) == "bundle";
                    continue;
                }
                if (!inBundleSection)
                {
                    continue;
                }
                const std::size_t separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    continue;
                }
                values[Trim(trimmed.substr(0, separator))] = Trim(trimmed.substr(separator + 1));
            }
            return values;
        }
    }

    void ProjectBundleManager::SaveManifest(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const
    {
        std::filesystem::create_directories(bundleDirectory);
        std::ofstream stream(bundleDirectory / "bundle.arsbundle");
        if (!stream)
        {
            throw std::runtime_error("Failed to write bundle manifest.");
        }

        stream << "; ApexRoboticsStudio bundle manifest\n\n"
               << "[bundle]\n"
               << "schema_version=" << manifest.SchemaVersion << '\n'
               << "bundle_name=" << manifest.BundleName << '\n'
               << "project_path=" << manifest.ProjectRelativePath.generic_string() << '\n'
               << "runtime_path=" << manifest.RuntimeRelativePath.generic_string() << '\n'
               << "plugin_directory=" << manifest.PluginDirectoryRelativePath.generic_string() << '\n'
               << "reports_directory=" << manifest.ReportsDirectoryRelativePath.generic_string() << '\n';
    }

    ProjectBundleManifest ProjectBundleManager::LoadManifest(const std::filesystem::path& bundleDirectory) const
    {
        const auto values = ParseBundleSection(bundleDirectory / "bundle.arsbundle");
        ProjectBundleManifest manifest;
        manifest.SchemaVersion = std::stoi(GetRequired(values, "schema_version"));
        manifest.BundleName = GetRequired(values, "bundle_name");
        manifest.ProjectRelativePath = GetRequired(values, "project_path");
        manifest.RuntimeRelativePath = GetRequired(values, "runtime_path");
        manifest.PluginDirectoryRelativePath = GetRequired(values, "plugin_directory");
        manifest.ReportsDirectoryRelativePath = GetRequired(values, "reports_directory");
        return manifest;
    }

    void ProjectBundleManager::CreateBundle(
        const std::filesystem::path& projectFilePath,
        const std::filesystem::path& runtimeConfigPath,
        const std::filesystem::path& outputDirectory,
        const std::filesystem::path* pluginDirectory) const
    {
        ProjectBundleManifest manifest;
        manifest.BundleName = outputDirectory.filename().string();

        const auto projectOut = outputDirectory / manifest.ProjectRelativePath;
        const auto runtimeOut = outputDirectory / manifest.RuntimeRelativePath;
        const auto pluginOut = outputDirectory / manifest.PluginDirectoryRelativePath;
        const auto reportsOut = outputDirectory / manifest.ReportsDirectoryRelativePath;

        std::filesystem::create_directories(projectOut.parent_path());
        std::filesystem::create_directories(runtimeOut.parent_path());
        std::filesystem::create_directories(pluginOut);
        std::filesystem::create_directories(reportsOut);

        std::filesystem::copy_file(projectFilePath, projectOut, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::copy_file(runtimeConfigPath, runtimeOut, std::filesystem::copy_options::overwrite_existing);
        if (pluginDirectory != nullptr && std::filesystem::exists(*pluginDirectory))
        {
            std::filesystem::copy(*pluginDirectory, pluginOut,
                std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
        }

        SaveManifest(manifest, outputDirectory);
    }

    std::filesystem::path ProjectBundleManager::ResolveProjectPath(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const
    {
        return bundleDirectory / manifest.ProjectRelativePath;
    }

    std::filesystem::path ProjectBundleManager::ResolveRuntimePath(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const
    {
        return bundleDirectory / manifest.RuntimeRelativePath;
    }

    std::filesystem::path ProjectBundleManager::ResolvePluginDirectory(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const
    {
        return bundleDirectory / manifest.PluginDirectoryRelativePath;
    }

    std::filesystem::path ProjectBundleManager::ResolveReportsDirectory(const ProjectBundleManifest& manifest, const std::filesystem::path& bundleDirectory) const
    {
        return bundleDirectory / manifest.ReportsDirectoryRelativePath;
    }
}
