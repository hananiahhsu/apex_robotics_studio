#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace apex::workflow
{
    struct FlowStep
    {
        std::string Name;
        std::string Action;
        std::string Argument;
    };

    struct AutomationFlow
    {
        int SchemaVersion = 1;
        std::string Name;
        std::filesystem::path BaseProjectPath;
        std::filesystem::path RuntimePath;
        std::filesystem::path RecipePath;
        std::filesystem::path PatchPath;
        std::filesystem::path ApprovalPath;
        std::filesystem::path PluginDirectory;
        std::filesystem::path OutputDirectory;
        std::vector<FlowStep> Steps;
    };

    class AutomationFlowSerializer
    {
    public:
        [[nodiscard]] AutomationFlow LoadFromFile(const std::filesystem::path& filePath) const;
        void SaveToFile(const AutomationFlow& flow, const std::filesystem::path& filePath) const;
    };
}
