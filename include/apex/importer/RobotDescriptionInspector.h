#pragma once

#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::importer
{
    struct RobotDescriptionInspection
    {
        std::string ProjectName;
        std::string RobotName;
        std::string SourceKind;
        std::string SourcePath;
        std::string ExpandedDescriptionPath;
        std::size_t PackageDependencyCount = 0;
        std::size_t MeshResourceCount = 0;
        std::size_t ResolvedMeshCount = 0;
        std::size_t MissingMeshCount = 0;
        std::vector<std::string> PackageDependencies;
        std::vector<std::string> MissingMeshUris;
        std::vector<std::string> DuplicateMeshUris;
        std::vector<std::string> Notes;
    };

    class RobotDescriptionInspector
    {
    public:
        [[nodiscard]] RobotDescriptionInspection Inspect(const apex::io::StudioProject& project) const;
        void WriteJsonReport(const RobotDescriptionInspection& inspection, const std::filesystem::path& outputPath) const;
        void WriteHtmlReport(const RobotDescriptionInspection& inspection, const std::filesystem::path& outputPath) const;
    };
}
