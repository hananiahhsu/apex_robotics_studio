#include "apex/edit/ProjectPatch.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace apex::edit
{
    namespace
    {
        using KeyValueMap = std::unordered_map<std::string, std::string>;

        struct ParsedSection
        {
            std::string Name;
            KeyValueMap Values;
        };

        std::string Trim(std::string value)
        {
            const auto isSpace = [](unsigned char character) { return std::isspace(character) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        std::string GetRequired(const KeyValueMap& values, const std::string& key, const std::string& sectionName)
        {
            const auto iterator = values.find(key);
            if (iterator == values.end())
            {
                throw std::invalid_argument("Missing key '" + key + "' in section '" + sectionName + "'.");
            }
            return iterator->second;
        }

        std::vector<std::string> Split(const std::string& text, char delimiter)
        {
            std::vector<std::string> tokens;
            std::stringstream stream(text);
            std::string token;
            while (std::getline(stream, token, delimiter))
            {
                tokens.push_back(Trim(token));
            }
            return tokens;
        }

        double ParseDouble(const std::string& text, const std::string& fieldName)
        {
            const std::string trimmed = Trim(text);
            char* end = nullptr;
            const double value = std::strtod(trimmed.c_str(), &end);
            if (end == trimmed.c_str() || *end != '\0')
            {
                throw std::invalid_argument("Invalid floating-point value for field '" + fieldName + "': '" + text + "'.");
            }
            return value;
        }

        std::size_t ParseSize(const std::string& text, const std::string& fieldName)
        {
            const std::string trimmed = Trim(text);
            int value = 0;
            const auto [pointer, errorCode] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
            if (errorCode != std::errc{} || pointer != trimmed.data() + trimmed.size() || value < 0)
            {
                throw std::invalid_argument("Invalid integer value for field '" + fieldName + "': '" + text + "'.");
            }
            return static_cast<std::size_t>(value);
        }

        apex::core::Vec3 ParseVec3(const std::string& text, const std::string& fieldName)
        {
            const auto parts = Split(text, ',');
            if (parts.size() != 3)
            {
                throw std::invalid_argument("Field '" + fieldName + "' must contain exactly three comma-separated values.");
            }
            return { ParseDouble(parts[0], fieldName + ".x"), ParseDouble(parts[1], fieldName + ".y"), ParseDouble(parts[2], fieldName + ".z") };
        }

        std::vector<ParsedSection> ParseSections(std::istream& stream)
        {
            std::vector<ParsedSection> sections;
            ParsedSection* currentSection = nullptr;
            std::string line;
            while (std::getline(stream, line))
            {
                const auto trimmed = Trim(line);
                if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#')
                {
                    continue;
                }
                if (trimmed.front() == '[' && trimmed.back() == ']')
                {
                    sections.push_back({ trimmed.substr(1, trimmed.size() - 2), {} });
                    currentSection = &sections.back();
                    continue;
                }
                if (currentSection == nullptr)
                {
                    throw std::invalid_argument("Patch content must be contained inside sections.");
                }
                const auto separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    throw std::invalid_argument("Patch line must use key=value syntax.");
                }
                currentSection->Values[Trim(trimmed.substr(0, separator))] = Trim(trimmed.substr(separator + 1));
            }
            return sections;
        }

        std::string FormatVec3(const apex::core::Vec3& value)
        {
            std::ostringstream builder;
            builder << value.X << ',' << value.Y << ',' << value.Z;
            return builder.str();
        }
    }

    ProjectPatch ProjectPatchLoader::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open patch file: " + filePath.string());
        }

        ProjectPatch patch;
        for (const auto& section : ParseSections(stream))
        {
            if (section.Name == "patch")
            {
                patch.Name = GetRequired(section.Values, "name", section.Name);
                const auto it = section.Values.find("notes");
                if (it != section.Values.end()) { patch.Notes = it->second; }
            }
            else if (section.Name == "metadata")
            {
                const auto it = section.Values.find("owner");
                if (it != section.Values.end()) { patch.OwnerOverride = it->second; }
            }
            else if (section.Name == "add_obstacle")
            {
                AddObstacleOperation operation;
                operation.Name = GetRequired(section.Values, "name", section.Name);
                operation.Min = ParseVec3(GetRequired(section.Values, "min", section.Name), "min");
                operation.Max = ParseVec3(GetRequired(section.Values, "max", section.Name), "max");
                patch.ObstaclesToAdd.push_back(operation);
            }
            else if (section.Name == "remove_obstacle")
            {
                patch.ObstaclesToRemove.push_back({ GetRequired(section.Values, "name", section.Name) });
            }
            else if (section.Name == "translate_obstacle")
            {
                TranslateObstacleOperation operation;
                operation.Name = GetRequired(section.Values, "name", section.Name);
                operation.Delta = ParseVec3(GetRequired(section.Values, "delta", section.Name), "delta");
                patch.ObstaclesToTranslate.push_back(operation);
            }
            else if (section.Name == "motion")
            {
                const auto itSamples = section.Values.find("sample_count");
                const auto itDuration = section.Values.find("duration_seconds");
                const auto itSafety = section.Values.find("link_safety_radius");
                if (itSamples != section.Values.end())
                {
                    patch.Motion.HasSampleCount = true;
                    patch.Motion.SampleCount = ParseSize(itSamples->second, "sample_count");
                }
                if (itDuration != section.Values.end())
                {
                    patch.Motion.HasDurationSeconds = true;
                    patch.Motion.DurationSeconds = ParseDouble(itDuration->second, "duration_seconds");
                }
                if (itSafety != section.Values.end())
                {
                    patch.Motion.HasLinkSafetyRadius = true;
                    patch.Motion.LinkSafetyRadius = ParseDouble(itSafety->second, "link_safety_radius");
                }
            }
        }
        return patch;
    }

    void ProjectPatchLoader::SaveToFile(const ProjectPatch& patch, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write patch file: " + filePath.string());
        }

        stream << "; ApexRoboticsStudio patch file\n\n[patch]\n"
               << "name=" << patch.Name << '\n'
               << "notes=" << patch.Notes << "\n\n";

        if (!patch.OwnerOverride.empty())
        {
            stream << "[metadata]\nowner=" << patch.OwnerOverride << "\n\n";
        }

        if (patch.Motion.HasSampleCount || patch.Motion.HasDurationSeconds || patch.Motion.HasLinkSafetyRadius)
        {
            stream << "[motion]\n";
            if (patch.Motion.HasSampleCount) { stream << "sample_count=" << patch.Motion.SampleCount << '\n'; }
            if (patch.Motion.HasDurationSeconds) { stream << "duration_seconds=" << patch.Motion.DurationSeconds << '\n'; }
            if (patch.Motion.HasLinkSafetyRadius) { stream << "link_safety_radius=" << patch.Motion.LinkSafetyRadius << '\n'; }
            stream << '\n';
        }

        for (const auto& operation : patch.ObstaclesToAdd)
        {
            stream << "[add_obstacle]\nname=" << operation.Name << '\n'
                   << "min=" << FormatVec3(operation.Min) << '\n'
                   << "max=" << FormatVec3(operation.Max) << "\n\n";
        }

        for (const auto& operation : patch.ObstaclesToRemove)
        {
            stream << "[remove_obstacle]\nname=" << operation.Name << "\n\n";
        }

        for (const auto& operation : patch.ObstaclesToTranslate)
        {
            stream << "[translate_obstacle]\nname=" << operation.Name << '\n'
                   << "delta=" << FormatVec3(operation.Delta) << "\n\n";
        }
    }

    apex::io::StudioProject ProjectEditor::ApplyPatch(const apex::io::StudioProject& project, const ProjectPatch& patch) const
    {
        apex::io::StudioProject result = project;
        if (!patch.OwnerOverride.empty())
        {
            result.Metadata.Owner = patch.OwnerOverride;
        }

        if (patch.Motion.HasSampleCount) { result.Motion.SampleCount = patch.Motion.SampleCount; }
        if (patch.Motion.HasDurationSeconds) { result.Motion.DurationSeconds = patch.Motion.DurationSeconds; }
        if (patch.Motion.HasLinkSafetyRadius)
        {
            result.Motion.LinkSafetyRadius = patch.Motion.LinkSafetyRadius;
            result.Job.LinkSafetyRadius = patch.Motion.LinkSafetyRadius;
        }

        for (const auto& removal : patch.ObstaclesToRemove)
        {
            result.Obstacles.erase(
                std::remove_if(result.Obstacles.begin(), result.Obstacles.end(), [&](const auto& obstacle) { return obstacle.Name == removal.Name; }),
                result.Obstacles.end());
        }

        for (const auto& translation : patch.ObstaclesToTranslate)
        {
            auto iterator = std::find_if(
                result.Obstacles.begin(),
                result.Obstacles.end(),
                [&](const auto& obstacle) { return obstacle.Name == translation.Name; });
            if (iterator == result.Obstacles.end())
            {
                throw std::invalid_argument("Cannot translate missing obstacle: " + translation.Name);
            }
            iterator->Bounds.Min = iterator->Bounds.Min + translation.Delta;
            iterator->Bounds.Max = iterator->Bounds.Max + translation.Delta;
        }

        for (const auto& addition : patch.ObstaclesToAdd)
        {
            result.Obstacles.push_back({ addition.Name, { addition.Min, addition.Max } });
        }

        if (!patch.Name.empty())
        {
            result.ProjectName += " | Patch: " + patch.Name;
        }
        if (!patch.Notes.empty())
        {
            if (!result.Metadata.Notes.empty())
            {
                result.Metadata.Notes += " | ";
            }
            result.Metadata.Notes += patch.Notes;
        }
        return result;
    }
}
