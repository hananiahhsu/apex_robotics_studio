#pragma once

#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::diff
{
    enum class DiffKind
    {
        Added,
        Removed,
        Modified
    };

    struct DiffEntry
    {
        DiffKind Kind = DiffKind::Modified;
        std::string Path;
        std::string BeforeValue;
        std::string AfterValue;
    };

    class ProjectDiffEngine
    {
    public:
        [[nodiscard]] std::vector<DiffEntry> Compare(
            const apex::io::StudioProject& lhs,
            const apex::io::StudioProject& rhs) const;

        [[nodiscard]] std::string BuildTextReport(const std::vector<DiffEntry>& entries) const;
        void SaveTextReport(const std::vector<DiffEntry>& entries, const std::filesystem::path& filePath) const;
    };

    [[nodiscard]] std::string ToString(DiffKind kind) noexcept;
}
