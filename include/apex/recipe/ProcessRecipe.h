#pragma once

#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>

namespace apex::recipe
{
    struct ProcessRecipe
    {
        std::string Name;
        std::string ProcessFamily;
        std::string Owner;
        std::string Notes;
        std::size_t DefaultSampleCount = 18;
        double SegmentDurationScale = 1.0;
        double LinkSafetyRadius = 0.08;
        double MaxPeakTcpSpeedMetersPerSecond = 1.6;
        double MaxAverageTcpSpeedMetersPerSecond = 1.0;
        double PreferredJointVelocityLimitRadPerSec = 1.3;
        std::size_t MaxWarningFindings = 2;
        bool RequireCollisionFree = true;
        std::string ProcessTagPrefix = "recipe";
    };

    class ProcessRecipeLoader
    {
    public:
        [[nodiscard]] ProcessRecipe LoadFromFile(const std::filesystem::path& filePath) const;
        void SaveToFile(const ProcessRecipe& recipe, const std::filesystem::path& filePath) const;
    };

    class ProcessRecipeApplicator
    {
    public:
        [[nodiscard]] apex::io::StudioProject ApplyRecipe(
            const apex::io::StudioProject& project,
            const ProcessRecipe& recipe) const;
    };
}
