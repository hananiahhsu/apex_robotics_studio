#pragma once

#include "apex/io/StudioProject.h"

#include <string>
#include <vector>

namespace apex::catalog
{
    struct CatalogItem
    {
        int Depth = 0;
        std::string Kind;
        std::string Name;
        std::string Details;
    };

    struct ProjectCatalog
    {
        std::string ProjectName;
        std::string RobotName;
        std::size_t JointCount = 0;
        std::size_t ObstacleCount = 0;
        std::size_t WaypointCount = 0;
        std::size_t SegmentCount = 0;
        std::size_t MeshCount = 0;
        std::vector<CatalogItem> Items;
    };

    class ProjectCatalogBuilder
    {
    public:
        [[nodiscard]] ProjectCatalog Build(const apex::io::StudioProject& project) const;
        [[nodiscard]] std::string BuildTextTree(const ProjectCatalog& catalog) const;
    };
}
