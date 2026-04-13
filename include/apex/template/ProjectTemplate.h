#pragma once

#include "apex/core/MathTypes.h"
#include "apex/io/StudioProject.h"

#include <filesystem>
#include <string>

namespace apex::templating
{
    struct TemplateOverrides
    {
        std::string ProjectName;
        std::string CellName;
        std::string ProcessFamily;
        std::string Owner;
        std::string NotesSuffix;
    };

    struct ProjectTemplate
    {
        std::string Name;
        std::string Description;
        std::string DefaultProjectName;
        std::string DefaultCellName;
        std::string DefaultProcessFamily;
        std::string DefaultOwner;
        std::string DefaultNotesSuffix;
        std::size_t SampleCountOverride = 0;
        double DurationScale = 1.0;
        double LinkSafetyRadius = 0.0;
        apex::core::Vec3 ObstacleOffset;
        apex::io::StudioProject BaseProject;
    };

    class ProjectTemplateLoader
    {
    public:
        [[nodiscard]] ProjectTemplate LoadFromFile(const std::filesystem::path& filePath) const;
        void SaveToFile(const ProjectTemplate& projectTemplate, const std::filesystem::path& filePath) const;
    };

    class ProjectTemplateInstantiator
    {
    public:
        [[nodiscard]] apex::io::StudioProject Instantiate(
            const ProjectTemplate& projectTemplate,
            const TemplateOverrides& overrides = {}) const;
    };
}
