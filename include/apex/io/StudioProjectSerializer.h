#pragma once

#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>

namespace apex::io
{
    class StudioProjectSerializer
    {
    public:
        [[nodiscard]] StudioProject LoadFromFile(const std::filesystem::path& filePath) const;
        [[nodiscard]] StudioProject LoadFromString(const std::string& text) const;
        void SaveToFile(const StudioProject& project, const std::filesystem::path& filePath) const;
        [[nodiscard]] std::string SaveToString(const StudioProject& project) const;
    };
}
