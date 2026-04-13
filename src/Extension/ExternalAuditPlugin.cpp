#include "apex/extension/ExternalAuditPlugin.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace apex::extension
{
    namespace
    {
        using KeyValueMap = std::unordered_map<std::string, std::string>;

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

        KeyValueMap ParsePluginSection(const std::filesystem::path& filePath)
        {
            std::ifstream stream(filePath);
            if (!stream)
            {
                throw std::runtime_error("Failed to open plugin manifest: " + filePath.string());
            }

            KeyValueMap values;
            std::string line;
            bool inPluginSection = false;
            while (std::getline(stream, line))
            {
                const std::string trimmed = Trim(line);
                if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';')
                {
                    continue;
                }
                if (trimmed.front() == '[' && trimmed.back() == ']')
                {
                    const std::string sectionName = trimmed.substr(1, trimmed.size() - 2);
                    inPluginSection = sectionName == "plugin";
                    continue;
                }
                if (!inPluginSection)
                {
                    continue;
                }
                const std::size_t separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    continue;
                }
                const std::string key = Trim(trimmed.substr(0, separator));
                const std::string value = Trim(trimmed.substr(separator + 1));
                values[key] = value;
            }
            return values;
        }

        std::string ReplaceAll(std::string text, const std::string& from, const std::string& to)
        {
            std::size_t position = 0;
            while ((position = text.find(from, position)) != std::string::npos)
            {
                text.replace(position, from.size(), to);
                position += to.size();
            }
            return text;
        }

        std::string QuoteArg(const std::filesystem::path& path)
        {
            return '"' + path.string() + '"';
        }

        FindingSeverity ParseSeverity(std::string text)
        {
            std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            if (text == "info") return FindingSeverity::Info;
            if (text == "warning" || text == "warn") return FindingSeverity::Warning;
            if (text == "error") return FindingSeverity::Error;
            throw std::invalid_argument("Unsupported plugin finding severity: " + text);
        }
    }

    std::vector<ExternalAuditPluginManifest> ExternalAuditPluginRunner::Discover(const std::filesystem::path& pluginDirectory) const
    {
        std::vector<ExternalAuditPluginManifest> manifests;
        if (!std::filesystem::exists(pluginDirectory))
        {
            return manifests;
        }

        for (const auto& entry : std::filesystem::directory_iterator(pluginDirectory))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".arsplugin")
            {
                continue;
            }

            const auto values = ParsePluginSection(entry.path());
            ExternalAuditPluginManifest manifest;
            manifest.Name = GetRequired(values, "name", "plugin");
            const std::filesystem::path executablePath = GetRequired(values, "executable", "plugin");
            manifest.ExecutablePath = executablePath.is_absolute() ? executablePath : (pluginDirectory / executablePath).lexically_normal();
            manifest.ArgumentsTemplate = GetRequired(values, "arguments", "plugin");
            manifest.ManifestPath = entry.path();
            manifests.push_back(std::move(manifest));
        }
        std::sort(manifests.begin(), manifests.end(), [](const auto& lhs, const auto& rhs){ return lhs.Name < rhs.Name; });
        return manifests;
    }

    std::vector<PluginFinding> ExternalAuditPluginRunner::ParseFindingsFile(
        const std::string& pluginName,
        const std::filesystem::path& findingsFilePath) const
    {
        std::ifstream stream(findingsFilePath);
        if (!stream)
        {
            throw std::runtime_error("Plugin '" + pluginName + "' did not produce findings file: " + findingsFilePath.string());
        }

        std::vector<PluginFinding> findings;
        std::string line;
        while (std::getline(stream, line))
        {
            const std::string trimmed = Trim(line);
            if (trimmed.empty())
            {
                continue;
            }
            const std::size_t separator = trimmed.find('|');
            if (separator == std::string::npos)
            {
                throw std::invalid_argument("Invalid plugin findings line: " + trimmed);
            }
            PluginFinding finding;
            finding.Severity = ParseSeverity(Trim(trimmed.substr(0, separator)));
            finding.Message = '[' + pluginName + "] " + Trim(trimmed.substr(separator + 1));
            findings.push_back(std::move(finding));
        }
        return findings;
    }

    std::vector<PluginFinding> ExternalAuditPluginRunner::Run(
        const std::filesystem::path& projectFilePath,
        const std::filesystem::path& pluginDirectory) const
    {
        std::vector<PluginFinding> findings;
        for (const auto& manifest : Discover(pluginDirectory))
        {
            if (!std::filesystem::exists(manifest.ExecutablePath))
            {
                findings.push_back({FindingSeverity::Error, '[' + manifest.Name + "] executable not found: " + manifest.ExecutablePath.string()});
                continue;
            }

            const std::filesystem::path findingsFilePath = std::filesystem::temp_directory_path() / (manifest.Name + "_findings.txt");
            std::string arguments = manifest.ArgumentsTemplate;
            arguments = ReplaceAll(arguments, "{project}", QuoteArg(projectFilePath));
            arguments = ReplaceAll(arguments, "{output}", QuoteArg(findingsFilePath));
            const std::string command = QuoteArg(manifest.ExecutablePath) + ' ' + arguments;
            const int exitCode = std::system(command.c_str());
            if (exitCode != 0)
            {
                std::ostringstream builder;
                builder << '[' << manifest.Name << "] plugin process failed with exit code " << exitCode << '.';
                findings.push_back({FindingSeverity::Error, builder.str()});
                continue;
            }

            auto pluginFindings = ParseFindingsFile(manifest.Name, findingsFilePath);
            findings.insert(findings.end(), pluginFindings.begin(), pluginFindings.end());

            std::error_code errorCode;
            std::filesystem::remove(findingsFilePath, errorCode);
        }
        return findings;
    }
}
