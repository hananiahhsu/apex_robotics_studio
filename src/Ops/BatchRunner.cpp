#include "apex/ops/BatchRunner.h"

#include "apex/core/SerialRobotModel.h"
#include "apex/extension/ExternalAuditPlugin.h"
#include "apex/extension/ProjectAuditPlugin.h"
#include "apex/io/StudioProjectSerializer.h"
#include "apex/platform/RuntimeConfig.h"
#include "apex/simulation/JobSimulationEngine.h"
#include "apex/workcell/CollisionWorld.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace apex::ops
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
            const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        std::vector<ParsedSection> ParseSections(std::istream& stream)
        {
            std::vector<ParsedSection> sections;
            ParsedSection* currentSection = nullptr;
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
                    currentSection = &sections.back();
                    continue;
                }
                if (currentSection == nullptr)
                {
                    continue;
                }
                const std::size_t separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    continue;
                }
                currentSection->Values[Trim(trimmed.substr(0, separator))] = Trim(trimmed.substr(separator + 1));
            }
            return sections;
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

        apex::core::SerialRobotModel BuildRobotFromProject(const apex::io::StudioProject& project)
        {
            apex::core::SerialRobotModel robot(project.RobotName);
            for (const auto& joint : project.Joints)
            {
                robot.AddRevoluteJoint(joint);
            }
            return robot;
        }

        apex::workcell::CollisionWorld BuildWorldFromProject(const apex::io::StudioProject& project)
        {
            apex::workcell::CollisionWorld world;
            for (const auto& obstacle : project.Obstacles)
            {
                world.AddObstacle(obstacle);
            }
            return world;
        }

        std::string EscapeHtml(std::string value)
        {
            const auto replaceAll = [&](const std::string& from, const std::string& to)
            {
                std::size_t position = 0;
                while ((position = value.find(from, position)) != std::string::npos)
                {
                    value.replace(position, from.size(), to);
                    position += to.size();
                }
            };
            replaceAll("&", "&amp;");
            replaceAll("<", "&lt;");
            replaceAll(">", "&gt;");
            replaceAll("\"", "&quot;");
            return value;
        }
    }

    BatchManifest BatchManifestLoader::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open batch manifest: " + filePath.string());
        }

        const auto sections = ParseSections(stream);
        BatchManifest manifest;
        const std::filesystem::path baseDirectory = filePath.has_parent_path() ? std::filesystem::absolute(filePath.parent_path()) : std::filesystem::current_path();
        for (const auto& section : sections)
        {
            if (section.Name == "batch")
            {
                manifest.SchemaVersion = std::stoi(GetRequired(section.Values, "schema_version", section.Name));
                manifest.Name = GetRequired(section.Values, "name", section.Name);
            }
            else if (section.Name == "entry")
            {
                BatchEntry entry;
                entry.Name = GetRequired(section.Values, "name", section.Name);
                entry.ProjectPath = (baseDirectory / GetRequired(section.Values, "project", section.Name)).lexically_normal();
                if (const auto it = section.Values.find("runtime"); it != section.Values.end() && !it->second.empty())
                {
                    entry.RuntimeConfigPath = (baseDirectory / it->second).lexically_normal();
                }
                if (const auto it = section.Values.find("plugin_directory"); it != section.Values.end() && !it->second.empty())
                {
                    entry.PluginDirectoryPath = (baseDirectory / it->second).lexically_normal();
                }
                manifest.Entries.push_back(std::move(entry));
            }
        }
        if (manifest.Name.empty())
        {
            throw std::invalid_argument("Batch manifest is missing [batch] section.");
        }
        return manifest;
    }

    void BatchManifestLoader::SaveToFile(const BatchManifest& manifest, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write batch manifest: " + filePath.string());
        }

        const std::filesystem::path baseDirectory = filePath.has_parent_path() ? std::filesystem::absolute(filePath.parent_path()) : std::filesystem::current_path();
        stream << "; ApexRoboticsStudio batch manifest\n\n"
               << "[batch]\n"
               << "schema_version=" << manifest.SchemaVersion << '\n'
               << "name=" << manifest.Name << "\n\n";
        for (const auto& entry : manifest.Entries)
        {
            stream << "[entry]\n"
                   << "name=" << entry.Name << '\n'
                   << "project=" << std::filesystem::relative(entry.ProjectPath, baseDirectory).generic_string() << '\n';
            if (!entry.RuntimeConfigPath.empty())
            {
                stream << "runtime=" << std::filesystem::relative(entry.RuntimeConfigPath, baseDirectory).generic_string() << '\n';
            }
            if (!entry.PluginDirectoryPath.empty())
            {
                stream << "plugin_directory=" << std::filesystem::relative(entry.PluginDirectoryPath, baseDirectory).generic_string() << '\n';
            }
            stream << '\n';
        }
    }

    BatchRunResult BatchRunner::Run(const BatchManifest& manifest) const
    {
        apex::io::StudioProjectSerializer serializer;
        apex::platform::RuntimeConfigLoader configLoader;
        apex::simulation::JobSimulationEngine engine;
        apex::extension::ProjectAuditPluginRegistry builtInRegistry;
        apex::extension::ExternalAuditPluginRunner externalRunner;

        BatchRunResult result;
        result.Name = manifest.Name;

        for (const auto& entry : manifest.Entries)
        {
            BatchEntryResult entryResult;
            entryResult.Entry = entry;
            try
            {
                const auto project = serializer.LoadFromFile(entry.ProjectPath);
                if (project.Job.Empty())
                {
                    throw std::invalid_argument("Project does not contain a job.");
                }

                double preferredVelocity = 1.5;
                bool enablePlugins = true;
                if (!entry.RuntimeConfigPath.empty())
                {
                    const auto config = configLoader.LoadFromFile(entry.RuntimeConfigPath);
                    preferredVelocity = config.PreferredJointVelocityLimitRadPerSec;
                    enablePlugins = config.EnableAuditPlugins;
                }

                const auto robot = BuildRobotFromProject(project);
                const auto world = BuildWorldFromProject(project);
                const auto simResult = engine.Simulate(robot, world, project.Job, preferredVelocity);

                std::vector<apex::extension::PluginFinding> findings;
                if (enablePlugins)
                {
                    const auto builtInFindings = builtInRegistry.RunBuiltInAudit(project, robot, world);
                    findings.insert(findings.end(), builtInFindings.begin(), builtInFindings.end());
                    if (!entry.PluginDirectoryPath.empty())
                    {
                        const auto externalFindings = externalRunner.Run(entry.ProjectPath, entry.PluginDirectoryPath);
                        findings.insert(findings.end(), externalFindings.begin(), externalFindings.end());
                    }
                }

                entryResult.Success = true;
                entryResult.CollisionFree = simResult.CollisionFree;
                entryResult.TotalDurationSeconds = simResult.TotalDurationSeconds;
                entryResult.TotalPathLengthMeters = simResult.TotalPathLengthMeters;
                entryResult.FindingCount = findings.size();
                entryResult.Warnings = simResult.Warnings;
                ++result.SuccessCount;
            }
            catch (const std::exception& exception)
            {
                entryResult.Success = false;
                entryResult.Warnings.push_back(exception.what());
                ++result.FailureCount;
            }
            result.Entries.push_back(std::move(entryResult));
        }

        return result;
    }

    std::string BatchRunner::BuildHtmlReport(const BatchRunResult& result) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head><meta charset=\"utf-8\"/>"
             << "<title>Apex Robotics Studio Batch Report</title>"
             << "<style>body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:24px;}"
             << ".card{background:#111827;border:1px solid #334155;border-radius:14px;padding:16px;margin-bottom:18px;}"
             << ".grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:16px;margin-bottom:18px;}"
             << "table{border-collapse:collapse;width:100%;}th,td{border-bottom:1px solid #334155;padding:10px;text-align:left;}"
             << ".good{color:#4ade80;}.bad{color:#f87171;}.warn{color:#fbbf24;}</style></head><body>\n";
        html << "<h1>Apex Robotics Studio - Batch Validation Report</h1>\n";
        html << "<p>Batch: <strong>" << EscapeHtml(result.Name) << "</strong></p>\n";
        html << "<div class=\"grid\">"
             << "<div class=\"card\"><h2>Entries</h2><div>Total: " << result.Entries.size() << "</div></div>"
             << "<div class=\"card\"><h2>Success</h2><div class=\"good\">" << result.SuccessCount << "</div></div>"
             << "<div class=\"card\"><h2>Failures</h2><div class=\"bad\">" << result.FailureCount << "</div></div>"
             << "</div>\n";
        html << "<div class=\"card\"><h2>Entries</h2><table><thead><tr><th>Name</th><th>Status</th><th>Collision Free</th><th>Duration</th><th>Path Length</th><th>Findings</th><th>Warnings</th></tr></thead><tbody>\n";
        for (const auto& entry : result.Entries)
        {
            html << "<tr><td>" << EscapeHtml(entry.Entry.Name) << "</td>"
                 << "<td class=\"" << (entry.Success ? "good" : "bad") << "\">" << (entry.Success ? "OK" : "Failed") << "</td>"
                 << "<td class=\"" << (entry.CollisionFree ? "good" : "warn") << "\">" << (entry.CollisionFree ? "YES" : "NO") << "</td>"
                 << "<td>" << entry.TotalDurationSeconds << " s</td>"
                 << "<td>" << entry.TotalPathLengthMeters << " m</td>"
                 << "<td>" << entry.FindingCount << "</td><td>";
            if (entry.Warnings.empty())
            {
                html << "<span class=\"good\">None</span>";
            }
            else
            {
                for (std::size_t i = 0; i < entry.Warnings.size(); ++i)
                {
                    if (i > 0) html << "<br/>";
                    html << "<span class=\"warn\">" << EscapeHtml(entry.Warnings[i]) << "</span>";
                }
            }
            html << "</td></tr>\n";
        }
        html << "</tbody></table></div></body></html>\n";
        return html.str();
    }

    void BatchRunner::SaveHtmlReport(const BatchRunResult& result, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write batch report: " + filePath.string());
        }
        stream << BuildHtmlReport(result);
    }
}
