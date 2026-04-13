#pragma once

#include "apex/core/SerialRobotModel.h"
#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

namespace apex::importer
{
    struct UrdfImportOptions
    {
        std::string RobotNameOverride;
        double DefaultJointLimitRad = 3.14159265358979323846;
        double DefaultLinkLengthMeters = 0.25;
        bool UseFullOriginNormForLinkLength = true;
        bool ResolveXacro = true;
        bool PreferExternalXacroTool = true;
        bool CaptureMeshResources = true;
        bool ResolvePackageUris = true;
        std::filesystem::path ResourceBaseDirectory;
        std::vector<std::filesystem::path> XacroIncludeDirectories;
        std::vector<std::filesystem::path> PackageSearchRoots;
        std::unordered_map<std::string, std::string> XacroArguments;
    };

    struct UrdfImportResult
    {
        apex::core::SerialRobotModel Robot = apex::core::SerialRobotModel("URDF Robot");
        apex::io::StudioProject Project;
        std::vector<std::string> Warnings;
        std::size_t TotalJointElements = 0;
        std::size_t ImportedRevoluteJointCount = 0;
        bool UsedXacro = false;
        bool UsedExternalXacroTool = false;
        std::size_t ResolvedMeshResourceCount = 0;
        std::string ExpandedRobotXml;
    };

    class UrdfImporter
    {
    public:
        [[nodiscard]] UrdfImportResult ImportFromFile(
            const std::filesystem::path& filePath,
            const UrdfImportOptions& options = {}) const;
    };
}
