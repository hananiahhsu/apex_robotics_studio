#pragma once

#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::edit
{
    struct AddObstacleOperation
    {
        std::string Name;
        apex::core::Vec3 Min;
        apex::core::Vec3 Max;
    };

    struct RemoveObstacleOperation
    {
        std::string Name;
    };

    struct TranslateObstacleOperation
    {
        std::string Name;
        apex::core::Vec3 Delta;
    };

    struct MotionOverride
    {
        bool HasSampleCount = false;
        std::size_t SampleCount = 0;
        bool HasDurationSeconds = false;
        double DurationSeconds = 0.0;
        bool HasLinkSafetyRadius = false;
        double LinkSafetyRadius = 0.0;
    };

    struct ProjectPatch
    {
        std::string Name;
        std::string Notes;
        std::string OwnerOverride;
        std::vector<AddObstacleOperation> ObstaclesToAdd;
        std::vector<RemoveObstacleOperation> ObstaclesToRemove;
        std::vector<TranslateObstacleOperation> ObstaclesToTranslate;
        MotionOverride Motion;
    };

    class ProjectPatchLoader
    {
    public:
        [[nodiscard]] ProjectPatch LoadFromFile(const std::filesystem::path& filePath) const;
        void SaveToFile(const ProjectPatch& patch, const std::filesystem::path& filePath) const;
    };

    class ProjectEditor
    {
    public:
        [[nodiscard]] apex::io::StudioProject ApplyPatch(
            const apex::io::StudioProject& project,
            const ProjectPatch& patch) const;
    };
}
