#pragma once

#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::integration
{
    struct Ros2WorkspaceExportOptions
    {
        std::string WorkspaceName = "apex_ros2_ws";
        std::string PackageStem;
        std::string PlanningGroupName = "manipulator";
        bool IncludeMoveItSkeleton = true;
        bool IncludeMoveItTaskSkeleton = true;
        bool IncludeDescriptionPackage = true;
        bool IncludeRos2ControlSkeleton = true;
        bool CopyMeshResources = true;
        bool RewriteMeshUris = true;
    };

    struct Ros2WorkspaceExportResult
    {
        std::filesystem::path WorkspaceRoot;
        std::filesystem::path DescriptionPackageRoot;
        std::filesystem::path BringupPackageRoot;
        std::filesystem::path MoveItPackageRoot;
        std::filesystem::path MoveItTaskPackageRoot;
        std::string DescriptionPackageName;
        std::string MoveItTaskPackageName;
        std::vector<std::filesystem::path> GeneratedFiles;
    };

    class Ros2WorkspaceExporter
    {
    public:
        [[nodiscard]] Ros2WorkspaceExportResult ExportWorkspace(
            const apex::io::StudioProject& project,
            const std::filesystem::path& outputDirectory,
            const Ros2WorkspaceExportOptions& options = {}) const;
    };
}
