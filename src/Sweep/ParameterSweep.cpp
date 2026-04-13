#include "apex/sweep/ParameterSweep.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace apex::sweep
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
                    throw std::invalid_argument("Sweep file contains key-value pair before any section.");
                }
                const std::size_t separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    throw std::invalid_argument("Sweep file line is not key=value: " + trimmed);
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

        double ParseDouble(const std::string& text, const std::string& fieldName)
        {
            const std::string trimmed = Trim(text);
            char* end = nullptr;
            const double value = std::strtod(trimmed.c_str(), &end);
            if (end == trimmed.c_str() || *end != '\0')
            {
                throw std::invalid_argument("Invalid double value for " + fieldName + ": " + text);
            }
            return value;
        }

        std::vector<std::string> Split(const std::string& text, char delimiter)
        {
            std::vector<std::string> tokens;
            std::stringstream stream(text);
            std::string token;
            while (std::getline(stream, token, delimiter))
            {
                const auto trimmed = Trim(token);
                if (!trimmed.empty())
                {
                    tokens.push_back(trimmed);
                }
            }
            return tokens;
        }

        std::vector<std::size_t> ParseSizes(const std::string& text)
        {
            std::vector<std::size_t> values;
            for (const auto& token : Split(text, ','))
            {
                const int parsed = ParseInt(token, "sample_counts");
                if (parsed < 0)
                {
                    throw std::invalid_argument("Sample counts must be non-negative.");
                }
                values.push_back(static_cast<std::size_t>(parsed));
            }
            return values;
        }

        std::vector<double> ParseDoubles(const std::string& text, const std::string& fieldName)
        {
            std::vector<double> values;
            for (const auto& token : Split(text, ','))
            {
                values.push_back(ParseDouble(token, fieldName));
            }
            return values;
        }

        std::string JoinSizes(const std::vector<std::size_t>& values)
        {
            std::ostringstream builder;
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                if (i > 0) { builder << ','; }
                builder << values[i];
            }
            return builder.str();
        }

        std::string JoinDoubles(const std::vector<double>& values)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(6);
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                if (i > 0) { builder << ','; }
                builder << values[i];
            }
            return builder.str();
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

    SweepDefinition ParameterSweepSerializer::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open sweep file: " + filePath.string());
        }
        const auto sections = ParseSections(stream);
        SweepDefinition sweep;
        for (const auto& section : sections)
        {
            if (section.Name == "sweep")
            {
                sweep.SchemaVersion = ParseInt(GetRequired(section.Values, "schema_version", section.Name), "schema_version");
                sweep.Name = GetRequired(section.Values, "name", section.Name);
                sweep.BaseProjectPath = GetRequired(section.Values, "base_project", section.Name);
                const auto itRuntime = section.Values.find("runtime");
                const auto itPlugins = section.Values.find("plugin_dir");
                const auto itOutput = section.Values.find("output_dir");
                if (itRuntime != section.Values.end()) { sweep.RuntimePath = itRuntime->second; }
                if (itPlugins != section.Values.end()) { sweep.PluginDirectory = itPlugins->second; }
                if (itOutput != section.Values.end()) { sweep.OutputDirectory = itOutput->second; }
                sweep.SampleCounts = ParseSizes(GetRequired(section.Values, "sample_counts", section.Name));
                sweep.DurationScales = ParseDoubles(GetRequired(section.Values, "duration_scales", section.Name), "duration_scales");
                sweep.SafetyRadii = ParseDoubles(GetRequired(section.Values, "safety_radii", section.Name), "safety_radii");
            }
        }
        if (sweep.Name.empty())
        {
            throw std::invalid_argument("Sweep file does not contain a [sweep] section.");
        }
        return sweep;
    }

    void ParameterSweepSerializer::SaveToFile(const SweepDefinition& sweep, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to create sweep file: " + filePath.string());
        }
        stream << "; ApexRoboticsStudio parameter sweep\n"
               << "[sweep]\n"
               << "schema_version=" << sweep.SchemaVersion << '\n'
               << "name=" << sweep.Name << '\n'
               << "base_project=" << sweep.BaseProjectPath.generic_string() << '\n';
        if (!sweep.RuntimePath.empty()) { stream << "runtime=" << sweep.RuntimePath.generic_string() << '\n'; }
        if (!sweep.PluginDirectory.empty()) { stream << "plugin_dir=" << sweep.PluginDirectory.generic_string() << '\n'; }
        if (!sweep.OutputDirectory.empty()) { stream << "output_dir=" << sweep.OutputDirectory.generic_string() << '\n'; }
        stream << "sample_counts=" << JoinSizes(sweep.SampleCounts) << '\n'
               << "duration_scales=" << JoinDoubles(sweep.DurationScales) << '\n'
               << "safety_radii=" << JoinDoubles(sweep.SafetyRadii) << '\n';
    }

    void ParameterSweepReportBuilder::SaveCsv(const std::vector<SweepVariantSummary>& variants, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write sweep CSV: " + filePath.string());
        }
        stream << "variant_name,sample_count,duration_scale,safety_radius,gate_status,collision_free,finding_count,total_duration_seconds,total_path_length_meters\n";
        for (const auto& variant : variants)
        {
            stream << variant.VariantName << ',' << variant.SampleCount << ',' << variant.DurationScale << ',' << variant.SafetyRadius << ','
                   << variant.GateStatus << ',' << (variant.CollisionFree ? "true" : "false") << ',' << variant.FindingCount << ','
                   << variant.TotalDurationSeconds << ',' << variant.TotalPathLengthMeters << '\n';
        }
    }

    std::string ParameterSweepReportBuilder::BuildHtml(const SweepDefinition& sweep, const std::vector<SweepVariantSummary>& variants) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"/>"
             << "<title>Apex Parameter Sweep</title>"
             << "<style>body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;padding:24px;}"
             << ".card{background:#111827;border:1px solid #334155;border-radius:14px;padding:16px;margin-bottom:16px;}"
             << "table{width:100%;border-collapse:collapse;}th,td{padding:8px;border-bottom:1px solid #334155;text-align:left;}"
             << ".pass{color:#4ade80;}.hold{color:#fbbf24;}.fail{color:#f87171;}</style></head><body>";
        html << "<h1>Parameter Sweep</h1><div class=\"card\"><div>Name: <strong>" << EscapeHtml(sweep.Name) << "</strong></div>"
             << "<div>Base Project: <strong>" << EscapeHtml(sweep.BaseProjectPath.generic_string()) << "</strong></div>"
             << "<div>Variants: <strong>" << variants.size() << "</strong></div></div>";
        html << "<div class=\"card\"><table><thead><tr><th>Variant</th><th>Samples</th><th>Duration Scale</th><th>Safety Radius</th><th>Gate</th><th>Collision Free</th><th>Findings</th><th>Total Duration</th><th>Path Length</th></tr></thead><tbody>";
        for (const auto& variant : variants)
        {
            html << "<tr><td>" << EscapeHtml(variant.VariantName) << "</td><td>" << variant.SampleCount << "</td><td>" << variant.DurationScale
                 << "</td><td>" << variant.SafetyRadius << "</td><td class=\"" << EscapeHtml(variant.GateStatus) << "\">" << EscapeHtml(variant.GateStatus)
                 << "</td><td>" << (variant.CollisionFree ? "true" : "false") << "</td><td>" << variant.FindingCount << "</td><td>"
                 << variant.TotalDurationSeconds << "</td><td>" << variant.TotalPathLengthMeters << "</td></tr>";
        }
        html << "</tbody></table></div></body></html>";
        return html.str();
    }

    void ParameterSweepReportBuilder::SaveHtml(const SweepDefinition& sweep, const std::vector<SweepVariantSummary>& variants, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write sweep HTML: " + filePath.string());
        }
        stream << BuildHtml(sweep, variants);
    }
}
