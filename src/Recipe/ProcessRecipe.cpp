#include "apex/recipe/ProcessRecipe.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace apex::recipe
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

        bool ParseBool(const std::string& text, const std::string& fieldName)
        {
            const std::string trimmed = Trim(text);
            if (trimmed == "true" || trimmed == "1" || trimmed == "yes" || trimmed == "on") { return true; }
            if (trimmed == "false" || trimmed == "0" || trimmed == "no" || trimmed == "off") { return false; }
            throw std::invalid_argument("Invalid boolean value for field '" + fieldName + "': '" + text + "'.");
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
                    throw std::invalid_argument("Recipe content must be inside sections.");
                }
                const auto separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    throw std::invalid_argument("Recipe line must use key=value syntax.");
                }
                currentSection->Values[Trim(trimmed.substr(0, separator))] = Trim(trimmed.substr(separator + 1));
            }
            return sections;
        }
    }

    ProcessRecipe ProcessRecipeLoader::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open recipe file: " + filePath.string());
        }

        ProcessRecipe recipe;
        for (const auto& section : ParseSections(stream))
        {
            if (section.Name == "recipe")
            {
                recipe.Name = GetRequired(section.Values, "name", section.Name);
                const auto itProcessFamily = section.Values.find("process_family");
                const auto itOwner = section.Values.find("owner");
                const auto itNotes = section.Values.find("notes");
                const auto itPrefix = section.Values.find("process_tag_prefix");
                if (itProcessFamily != section.Values.end()) { recipe.ProcessFamily = itProcessFamily->second; }
                if (itOwner != section.Values.end()) { recipe.Owner = itOwner->second; }
                if (itNotes != section.Values.end()) { recipe.Notes = itNotes->second; }
                if (itPrefix != section.Values.end()) { recipe.ProcessTagPrefix = itPrefix->second; }
            }
            else if (section.Name == "motion")
            {
                const auto itSampleCount = section.Values.find("default_sample_count");
                const auto itScale = section.Values.find("segment_duration_scale");
                const auto itSafety = section.Values.find("link_safety_radius");
                if (itSampleCount != section.Values.end()) { recipe.DefaultSampleCount = ParseSize(itSampleCount->second, "default_sample_count"); }
                if (itScale != section.Values.end()) { recipe.SegmentDurationScale = ParseDouble(itScale->second, "segment_duration_scale"); }
                if (itSafety != section.Values.end()) { recipe.LinkSafetyRadius = ParseDouble(itSafety->second, "link_safety_radius"); }
            }
            else if (section.Name == "quality_gate")
            {
                const auto itPeak = section.Values.find("max_peak_tcp_speed_mps");
                const auto itAverage = section.Values.find("max_average_tcp_speed_mps");
                const auto itVelocity = section.Values.find("preferred_joint_velocity_limit_rad_per_sec");
                const auto itWarnings = section.Values.find("max_warning_findings");
                const auto itCollision = section.Values.find("require_collision_free");
                if (itPeak != section.Values.end()) { recipe.MaxPeakTcpSpeedMetersPerSecond = ParseDouble(itPeak->second, "max_peak_tcp_speed_mps"); }
                if (itAverage != section.Values.end()) { recipe.MaxAverageTcpSpeedMetersPerSecond = ParseDouble(itAverage->second, "max_average_tcp_speed_mps"); }
                if (itVelocity != section.Values.end()) { recipe.PreferredJointVelocityLimitRadPerSec = ParseDouble(itVelocity->second, "preferred_joint_velocity_limit_rad_per_sec"); }
                if (itWarnings != section.Values.end()) { recipe.MaxWarningFindings = ParseSize(itWarnings->second, "max_warning_findings"); }
                if (itCollision != section.Values.end()) { recipe.RequireCollisionFree = ParseBool(itCollision->second, "require_collision_free"); }
            }
        }
        if (recipe.Name.empty())
        {
            throw std::invalid_argument("Recipe file must define [recipe] name.");
        }
        return recipe;
    }

    void ProcessRecipeLoader::SaveToFile(const ProcessRecipe& recipe, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write recipe file: " + filePath.string());
        }

        stream << "; ApexRoboticsStudio process recipe\n\n"
               << "[recipe]\n"
               << "name=" << recipe.Name << '\n'
               << "process_family=" << recipe.ProcessFamily << '\n'
               << "owner=" << recipe.Owner << '\n'
               << "notes=" << recipe.Notes << '\n'
               << "process_tag_prefix=" << recipe.ProcessTagPrefix << "\n\n"
               << "[motion]\n"
               << "default_sample_count=" << recipe.DefaultSampleCount << '\n'
               << "segment_duration_scale=" << recipe.SegmentDurationScale << '\n'
               << "link_safety_radius=" << recipe.LinkSafetyRadius << "\n\n"
               << "[quality_gate]\n"
               << "max_peak_tcp_speed_mps=" << recipe.MaxPeakTcpSpeedMetersPerSecond << '\n'
               << "max_average_tcp_speed_mps=" << recipe.MaxAverageTcpSpeedMetersPerSecond << '\n'
               << "preferred_joint_velocity_limit_rad_per_sec=" << recipe.PreferredJointVelocityLimitRadPerSec << '\n'
               << "max_warning_findings=" << recipe.MaxWarningFindings << '\n'
               << "require_collision_free=" << (recipe.RequireCollisionFree ? "true" : "false") << '\n';
    }

    apex::io::StudioProject ProcessRecipeApplicator::ApplyRecipe(const apex::io::StudioProject& project, const ProcessRecipe& recipe) const
    {
        apex::io::StudioProject result = project;
        result.Metadata.ProcessFamily = recipe.ProcessFamily;
        if (!recipe.Owner.empty())
        {
            result.Metadata.Owner = recipe.Owner;
        }
        if (!recipe.Notes.empty())
        {
            if (!result.Metadata.Notes.empty())
            {
                result.Metadata.Notes += " | ";
            }
            result.Metadata.Notes += recipe.Notes;
        }
        result.Motion.LinkSafetyRadius = recipe.LinkSafetyRadius;
        result.Job.LinkSafetyRadius = recipe.LinkSafetyRadius;
        result.QualityGate.RequireCollisionFree = recipe.RequireCollisionFree;
        result.QualityGate.MaxWarningFindings = recipe.MaxWarningFindings;
        result.QualityGate.MaxPeakTcpSpeedMetersPerSecond = recipe.MaxPeakTcpSpeedMetersPerSecond;
        result.QualityGate.MaxAverageTcpSpeedMetersPerSecond = recipe.MaxAverageTcpSpeedMetersPerSecond;
        result.QualityGate.PreferredJointVelocityLimitRadPerSec = recipe.PreferredJointVelocityLimitRadPerSec;

        if (!result.Job.Name.empty())
        {
            result.Job.Name += " | " + recipe.Name;
        }
        result.ProjectName += " | Recipe: " + recipe.Name;

        for (auto& segment : result.Job.Segments)
        {
            if (recipe.DefaultSampleCount > 0)
            {
                segment.SampleCount = recipe.DefaultSampleCount;
            }
            segment.DurationSeconds *= recipe.SegmentDurationScale;
            if (!recipe.ProcessTagPrefix.empty())
            {
                const std::string previousTag = segment.ProcessTag.empty() ? segment.Name : segment.ProcessTag;
                segment.ProcessTag = recipe.ProcessTagPrefix + "." + previousTag;
            }
        }
        return result;
    }
}
