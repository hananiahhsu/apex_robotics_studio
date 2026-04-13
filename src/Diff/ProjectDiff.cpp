#include "apex/diff/ProjectDiff.h"

#include "apex/core/MathTypes.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace apex::diff
{
    namespace
    {
        template <typename T>
        void AddIfChanged(
            std::vector<DiffEntry>& entries,
            const std::string& path,
            const T& lhs,
            const T& rhs)
        {
            if (lhs != rhs)
            {
                std::ostringstream before;
                std::ostringstream after;
                before << lhs;
                after << rhs;
                entries.push_back({ DiffKind::Modified, path, before.str(), after.str() });
            }
        }

        void AddOrRemoveEntries(
            std::vector<DiffEntry>& entries,
            const std::string& path,
            const std::unordered_map<std::string, std::string>& lhs,
            const std::unordered_map<std::string, std::string>& rhs)
        {
            for (const auto& [name, value] : lhs)
            {
                if (rhs.find(name) == rhs.end())
                {
                    entries.push_back({ DiffKind::Removed, path + "/" + name, value, "" });
                }
            }
            for (const auto& [name, value] : rhs)
            {
                const auto iterator = lhs.find(name);
                if (iterator == lhs.end())
                {
                    entries.push_back({ DiffKind::Added, path + "/" + name, "", value });
                }
                else if (iterator->second != value)
                {
                    entries.push_back({ DiffKind::Modified, path + "/" + name, iterator->second, value });
                }
            }
        }
    }

    std::string ToString(DiffKind kind) noexcept
    {
        switch (kind)
        {
        case DiffKind::Added: return "ADDED";
        case DiffKind::Removed: return "REMOVED";
        case DiffKind::Modified: return "MODIFIED";
        }
        return "UNKNOWN";
    }

    std::vector<DiffEntry> ProjectDiffEngine::Compare(const apex::io::StudioProject& lhs, const apex::io::StudioProject& rhs) const
    {
        std::vector<DiffEntry> entries;
        AddIfChanged(entries, "project/name", lhs.ProjectName, rhs.ProjectName);
        AddIfChanged(entries, "project/robot_name", lhs.RobotName, rhs.RobotName);
        AddIfChanged(entries, "metadata/cell_name", lhs.Metadata.CellName, rhs.Metadata.CellName);
        AddIfChanged(entries, "metadata/process_family", lhs.Metadata.ProcessFamily, rhs.Metadata.ProcessFamily);
        AddIfChanged(entries, "metadata/owner", lhs.Metadata.Owner, rhs.Metadata.Owner);
        AddIfChanged(entries, "motion/sample_count", lhs.Motion.SampleCount, rhs.Motion.SampleCount);
        AddIfChanged(entries, "motion/duration_seconds", lhs.Motion.DurationSeconds, rhs.Motion.DurationSeconds);
        AddIfChanged(entries, "motion/link_safety_radius", lhs.Motion.LinkSafetyRadius, rhs.Motion.LinkSafetyRadius);

        std::unordered_map<std::string, std::string> lhsObstacles;
        std::unordered_map<std::string, std::string> rhsObstacles;
        for (const auto& obstacle : lhs.Obstacles)
        {
            lhsObstacles[obstacle.Name] = apex::core::ToString(obstacle.Bounds.Min) + " -> " + apex::core::ToString(obstacle.Bounds.Max);
        }
        for (const auto& obstacle : rhs.Obstacles)
        {
            rhsObstacles[obstacle.Name] = apex::core::ToString(obstacle.Bounds.Min) + " -> " + apex::core::ToString(obstacle.Bounds.Max);
        }
        AddOrRemoveEntries(entries, "obstacles", lhsObstacles, rhsObstacles);

        std::unordered_map<std::string, std::string> lhsSegments;
        std::unordered_map<std::string, std::string> rhsSegments;
        for (const auto& segment : lhs.Job.Segments)
        {
            lhsSegments[segment.Name] = segment.StartWaypointName + "->" + segment.GoalWaypointName +
                " samples=" + std::to_string(segment.SampleCount) + " duration=" + std::to_string(segment.DurationSeconds) +
                " tag=" + segment.ProcessTag;
        }
        for (const auto& segment : rhs.Job.Segments)
        {
            rhsSegments[segment.Name] = segment.StartWaypointName + "->" + segment.GoalWaypointName +
                " samples=" + std::to_string(segment.SampleCount) + " duration=" + std::to_string(segment.DurationSeconds) +
                " tag=" + segment.ProcessTag;
        }
        AddOrRemoveEntries(entries, "job/segments", lhsSegments, rhsSegments);

        return entries;
    }

    std::string ProjectDiffEngine::BuildTextReport(const std::vector<DiffEntry>& entries) const
    {
        std::ostringstream report;
        report << "ApexRoboticsStudio Project Diff\n";
        report << "Entries: " << entries.size() << "\n\n";
        for (const auto& entry : entries)
        {
            report << '[' << ToString(entry.Kind) << "] " << entry.Path << "\n";
            if (!entry.BeforeValue.empty())
            {
                report << "  before: " << entry.BeforeValue << "\n";
            }
            if (!entry.AfterValue.empty())
            {
                report << "  after : " << entry.AfterValue << "\n";
            }
            report << '\n';
        }
        return report.str();
    }

    void ProjectDiffEngine::SaveTextReport(const std::vector<DiffEntry>& entries, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write diff report: " + filePath.string());
        }
        stream << BuildTextReport(entries);
    }
}
