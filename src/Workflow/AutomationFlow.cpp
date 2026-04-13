#include "apex/workflow/AutomationFlow.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace apex::workflow
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
            const auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
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
                if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';')
                {
                    continue;
                }
                if (trimmed.front() == '[' && trimmed.back() == ']')
                {
                    sections.push_back({trimmed.substr(1, trimmed.size() - 2), {}});
                    current = &sections.back();
                    continue;
                }
                if (current == nullptr)
                {
                    throw std::invalid_argument("Flow file contains key-value pair before any section.");
                }
                const std::size_t separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    throw std::invalid_argument("Flow file line is not key=value: " + trimmed);
                }
                current->Values[Trim(trimmed.substr(0, separator))] = Trim(trimmed.substr(separator + 1));
            }
            return sections;
        }

        std::string GetRequired(const KeyValueMap& values, const std::string& key, const std::string& section)
        {
            const auto it = values.find(key);
            if (it == values.end())
            {
                throw std::invalid_argument("Missing key '" + key + "' in section '" + section + "'.");
            }
            return it->second;
        }

        int ParseInt(const std::string& text, const std::string& fieldName)
        {
            int value = 0;
            const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
            if (ec != std::errc{} || ptr != text.data() + text.size())
            {
                throw std::invalid_argument("Invalid integer value for " + fieldName + ": " + text);
            }
            return value;
        }
    }

    AutomationFlow AutomationFlowSerializer::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open flow file: " + filePath.string());
        }

        const auto sections = ParseSections(stream);
        AutomationFlow flow;
        for (const auto& section : sections)
        {
            if (section.Name == "flow")
            {
                flow.SchemaVersion = ParseInt(GetRequired(section.Values, "schema_version", section.Name), "schema_version");
                flow.Name = GetRequired(section.Values, "name", section.Name);
                flow.BaseProjectPath = GetRequired(section.Values, "base_project", section.Name);
                const auto itRuntime = section.Values.find("runtime");
                const auto itRecipe = section.Values.find("recipe");
                const auto itPatch = section.Values.find("patch");
                const auto itApproval = section.Values.find("approval");
                const auto itPlugins = section.Values.find("plugin_dir");
                const auto itOutput = section.Values.find("output_dir");
                if (itRuntime != section.Values.end()) { flow.RuntimePath = itRuntime->second; }
                if (itRecipe != section.Values.end()) { flow.RecipePath = itRecipe->second; }
                if (itPatch != section.Values.end()) { flow.PatchPath = itPatch->second; }
                if (itApproval != section.Values.end()) { flow.ApprovalPath = itApproval->second; }
                if (itPlugins != section.Values.end()) { flow.PluginDirectory = itPlugins->second; }
                if (itOutput != section.Values.end()) { flow.OutputDirectory = itOutput->second; }
            }
            else if (section.Name == "step")
            {
                FlowStep step;
                step.Name = GetRequired(section.Values, "name", section.Name);
                step.Action = GetRequired(section.Values, "action", section.Name);
                const auto itArg = section.Values.find("argument");
                if (itArg != section.Values.end())
                {
                    step.Argument = itArg->second;
                }
                flow.Steps.push_back(std::move(step));
            }
        }

        if (flow.Name.empty())
        {
            throw std::invalid_argument("Flow file does not contain a [flow] section.");
        }
        return flow;
    }

    void AutomationFlowSerializer::SaveToFile(const AutomationFlow& flow, const std::filesystem::path& filePath) const
    {
        if (flow.Name.empty())
        {
            throw std::invalid_argument("Flow name must not be empty.");
        }
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to create flow file: " + filePath.string());
        }
        stream << "; ApexRoboticsStudio automation flow\n"
               << "[flow]\n"
               << "schema_version=" << flow.SchemaVersion << '\n'
               << "name=" << flow.Name << '\n'
               << "base_project=" << flow.BaseProjectPath.generic_string() << '\n';
        if (!flow.RuntimePath.empty()) { stream << "runtime=" << flow.RuntimePath.generic_string() << '\n'; }
        if (!flow.RecipePath.empty()) { stream << "recipe=" << flow.RecipePath.generic_string() << '\n'; }
        if (!flow.PatchPath.empty()) { stream << "patch=" << flow.PatchPath.generic_string() << '\n'; }
        if (!flow.ApprovalPath.empty()) { stream << "approval=" << flow.ApprovalPath.generic_string() << '\n'; }
        if (!flow.PluginDirectory.empty()) { stream << "plugin_dir=" << flow.PluginDirectory.generic_string() << '\n'; }
        if (!flow.OutputDirectory.empty()) { stream << "output_dir=" << flow.OutputDirectory.generic_string() << '\n'; }
        stream << '\n';
        for (const auto& step : flow.Steps)
        {
            stream << "[step]\n"
                   << "name=" << step.Name << '\n'
                   << "action=" << step.Action << '\n';
            if (!step.Argument.empty())
            {
                stream << "argument=" << step.Argument << '\n';
            }
            stream << '\n';
        }
    }
}
