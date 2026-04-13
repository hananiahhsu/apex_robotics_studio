#include "apex/template/ProjectTemplate.h"

#include "apex/io/StudioProjectSerializer.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace apex::templating
{
    namespace
    {
        using KeyValueMap = std::unordered_map<std::string, std::string>;

        struct ParsedSection
        {
            std::string Name;
            KeyValueMap Values;
            std::vector<std::string> Lines;
        };

        std::string Trim(std::string value)
        {
            const auto isSpace = [](unsigned char character) { return std::isspace(character) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        std::vector<ParsedSection> ParseSections(std::istream& stream)
        {
            std::vector<ParsedSection> sections;
            ParsedSection* current = nullptr;
            std::string line;
            while (std::getline(stream, line))
            {
                const std::string trimmed = Trim(line);
                if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '#')
                {
                    continue;
                }
                if (trimmed.front() == '[' && trimmed.back() == ']')
                {
                    sections.push_back({trimmed.substr(1, trimmed.size() - 2), {}, {}});
                    current = &sections.back();
                    current->Lines.push_back(trimmed);
                    continue;
                }
                if (current == nullptr)
                {
                    throw std::invalid_argument("Template content found before any section.");
                }
                const std::size_t pos = trimmed.find('=');
                if (pos == std::string::npos)
                {
                    throw std::invalid_argument("Expected key=value syntax in template file.");
                }
                const std::string key = Trim(trimmed.substr(0, pos));
                const std::string value = Trim(trimmed.substr(pos + 1));
                current->Values[key] = value;
                current->Lines.push_back(trimmed);
            }
            return sections;
        }

        std::string GetRequired(const KeyValueMap& values, const std::string& key)
        {
            const auto it = values.find(key);
            if (it == values.end())
            {
                throw std::invalid_argument("Missing key '" + key + "' in template file.");
            }
            return it->second;
        }

        std::size_t ParseSize(const std::string& text)
        {
            return static_cast<std::size_t>(std::stoul(text));
        }

        double ParseDouble(const std::string& text)
        {
            return std::stod(text);
        }

        apex::core::Vec3 ParseVec3(const std::string& text)
        {
            std::stringstream stream(text);
            std::string token;
            std::vector<double> values;
            while (std::getline(stream, token, ','))
            {
                values.push_back(std::stod(Trim(token)));
            }
            if (values.size() != 3)
            {
                throw std::invalid_argument("Expected three comma-separated values for vector field.");
            }
            return {values[0], values[1], values[2]};
        }

        std::string FormatVec3(const apex::core::Vec3& value)
        {
            std::ostringstream stream;
            stream.setf(std::ios::fixed);
            stream.precision(6);
            stream << value.X << ',' << value.Y << ',' << value.Z;
            return stream.str();
        }
    }

    ProjectTemplate ProjectTemplateLoader::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open project template file: " + filePath.string());
        }

        const auto sections = ParseSections(stream);
        ProjectTemplate projectTemplate;
        std::ostringstream projectBuilder;
        bool hasTemplateSection = false;

        for (const auto& section : sections)
        {
            if (section.Name == "template")
            {
                hasTemplateSection = true;
                projectTemplate.Name = GetRequired(section.Values, "name");
                projectTemplate.Description = section.Values.count("description") ? section.Values.at("description") : std::string{};
                projectTemplate.DefaultProjectName = section.Values.count("default_project_name") ? section.Values.at("default_project_name") : std::string{};
                projectTemplate.DefaultCellName = section.Values.count("default_cell_name") ? section.Values.at("default_cell_name") : std::string{};
                projectTemplate.DefaultProcessFamily = section.Values.count("default_process_family") ? section.Values.at("default_process_family") : std::string{};
                projectTemplate.DefaultOwner = section.Values.count("default_owner") ? section.Values.at("default_owner") : std::string{};
                projectTemplate.DefaultNotesSuffix = section.Values.count("default_notes_suffix") ? section.Values.at("default_notes_suffix") : std::string{};
                projectTemplate.SampleCountOverride = section.Values.count("sample_count_override") ? ParseSize(section.Values.at("sample_count_override")) : 0U;
                projectTemplate.DurationScale = section.Values.count("duration_scale") ? ParseDouble(section.Values.at("duration_scale")) : 1.0;
                projectTemplate.LinkSafetyRadius = section.Values.count("link_safety_radius") ? ParseDouble(section.Values.at("link_safety_radius")) : 0.0;
                projectTemplate.ObstacleOffset = section.Values.count("obstacle_offset") ? ParseVec3(section.Values.at("obstacle_offset")) : apex::core::Vec3{};
                continue;
            }

            projectBuilder << '[' << section.Name << "]\n";
            for (const auto& line : section.Lines)
            {
                if (!line.empty() && line.front() != '[')
                {
                    projectBuilder << line << '\n';
                }
            }
            projectBuilder << '\n';
        }

        if (!hasTemplateSection)
        {
            throw std::invalid_argument("Template file is missing [template] section.");
        }

        const apex::io::StudioProjectSerializer serializer;
        projectTemplate.BaseProject = serializer.LoadFromString(projectBuilder.str());
        return projectTemplate;
    }

    void ProjectTemplateLoader::SaveToFile(const ProjectTemplate& projectTemplate, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write project template file: " + filePath.string());
        }

        stream << "; ApexRoboticsStudio project template\n\n"
               << "[template]\n"
               << "name=" << projectTemplate.Name << '\n'
               << "description=" << projectTemplate.Description << '\n'
               << "default_project_name=" << projectTemplate.DefaultProjectName << '\n'
               << "default_cell_name=" << projectTemplate.DefaultCellName << '\n'
               << "default_process_family=" << projectTemplate.DefaultProcessFamily << '\n'
               << "default_owner=" << projectTemplate.DefaultOwner << '\n'
               << "default_notes_suffix=" << projectTemplate.DefaultNotesSuffix << '\n'
               << "sample_count_override=" << projectTemplate.SampleCountOverride << '\n'
               << "duration_scale=" << projectTemplate.DurationScale << '\n'
               << "link_safety_radius=" << projectTemplate.LinkSafetyRadius << '\n'
               << "obstacle_offset=" << FormatVec3(projectTemplate.ObstacleOffset) << "\n\n";

        const apex::io::StudioProjectSerializer serializer;
        stream << serializer.SaveToString(projectTemplate.BaseProject);
    }

    apex::io::StudioProject ProjectTemplateInstantiator::Instantiate(const ProjectTemplate& projectTemplate, const TemplateOverrides& overrides) const
    {
        auto project = projectTemplate.BaseProject;
        project.ProjectName = !overrides.ProjectName.empty() ? overrides.ProjectName
                           : (!projectTemplate.DefaultProjectName.empty() ? projectTemplate.DefaultProjectName : project.ProjectName);
        project.Metadata.CellName = !overrides.CellName.empty() ? overrides.CellName
                                 : (!projectTemplate.DefaultCellName.empty() ? projectTemplate.DefaultCellName : project.Metadata.CellName);
        project.Metadata.ProcessFamily = !overrides.ProcessFamily.empty() ? overrides.ProcessFamily
                                      : (!projectTemplate.DefaultProcessFamily.empty() ? projectTemplate.DefaultProcessFamily : project.Metadata.ProcessFamily);
        project.Metadata.Owner = !overrides.Owner.empty() ? overrides.Owner
                              : (!projectTemplate.DefaultOwner.empty() ? projectTemplate.DefaultOwner : project.Metadata.Owner);

        const std::string notesSuffix = !overrides.NotesSuffix.empty() ? overrides.NotesSuffix : projectTemplate.DefaultNotesSuffix;
        if (!notesSuffix.empty())
        {
            if (!project.Metadata.Notes.empty())
            {
                project.Metadata.Notes += " ";
            }
            project.Metadata.Notes += notesSuffix;
        }

        if (projectTemplate.SampleCountOverride > 0)
        {
            project.Motion.SampleCount = projectTemplate.SampleCountOverride;
            for (auto& segment : project.Job.Segments)
            {
                segment.SampleCount = projectTemplate.SampleCountOverride;
            }
        }

        if (projectTemplate.DurationScale > 0.0)
        {
            project.Motion.DurationSeconds *= projectTemplate.DurationScale;
            for (auto& segment : project.Job.Segments)
            {
                segment.DurationSeconds *= projectTemplate.DurationScale;
            }
        }

        if (projectTemplate.LinkSafetyRadius > 0.0)
        {
            project.Motion.LinkSafetyRadius = projectTemplate.LinkSafetyRadius;
            project.Job.LinkSafetyRadius = projectTemplate.LinkSafetyRadius;
        }

        for (auto& obstacle : project.Obstacles)
        {
            obstacle.Bounds.Min = obstacle.Bounds.Min + projectTemplate.ObstacleOffset;
            obstacle.Bounds.Max = obstacle.Bounds.Max + projectTemplate.ObstacleOffset;
        }

        return project;
    }
}
