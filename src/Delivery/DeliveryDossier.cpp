#include "apex/delivery/DeliveryDossier.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace apex::delivery
{
    namespace
    {
        std::string JsonEscape(const std::string& value)
        {
            std::string output;
            output.reserve(value.size());
            for (const char ch : value)
            {
                switch (ch)
                {
                case '\\': output += "\\\\"; break;
                case '"': output += "\\\""; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default: output += ch; break;
                }
            }
            return output;
        }
    }

    void DeliveryDossierBuilder::SaveManifestJson(const DeliveryDossierManifest& manifest, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write delivery dossier manifest: " + filePath.string());
        }

        stream << "{\n"
               << "  \"title\": \"" << JsonEscape(manifest.Title) << "\",\n"
               << "  \"project_name\": \"" << JsonEscape(manifest.ProjectName) << "\",\n"
               << "  \"gate_status\": \"" << apex::quality::ToString(manifest.GateStatus) << "\",\n"
               << "  \"session_status\": \"" << apex::session::ToString(manifest.SessionStatus) << "\",\n"
               << "  \"artifacts\": [\n";
        for (std::size_t index = 0; index < manifest.Artifacts.size(); ++index)
        {
            const auto& artifact = manifest.Artifacts[index];
            stream << "    {\"type\": \"" << JsonEscape(artifact.Type)
                   << "\", \"relative_path\": \"" << JsonEscape(artifact.RelativePath.generic_string())
                   << "\", \"description\": \"" << JsonEscape(artifact.Description) << "\"}";
            if (index + 1 < manifest.Artifacts.size())
            {
                stream << ',';
            }
            stream << '\n';
        }
        stream << "  ]\n}";
    }

    std::string DeliveryDossierBuilder::BuildIndexHtml(const DeliveryDossierManifest& manifest) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Delivery Dossier</title>"
             << "<style>body{font-family:Arial,sans-serif;margin:24px;}table{border-collapse:collapse;width:100%;}"
             << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}th{background:#f5f5f5;}</style></head><body>";
        html << "<h1>" << manifest.Title << "</h1>";
        html << "<p><strong>Project:</strong> " << manifest.ProjectName << "<br><strong>Gate:</strong> "
             << apex::quality::ToString(manifest.GateStatus) << "<br><strong>Session:</strong> "
             << apex::session::ToString(manifest.SessionStatus) << "</p>";
        html << "<table><thead><tr><th>Type</th><th>Artifact</th><th>Description</th></tr></thead><tbody>";
        for (const auto& artifact : manifest.Artifacts)
        {
            html << "<tr><td>" << artifact.Type << "</td><td><a href=\"" << artifact.RelativePath.generic_string() << "\">"
                 << artifact.RelativePath.generic_string() << "</a></td><td>" << artifact.Description << "</td></tr>";
        }
        html << "</tbody></table></body></html>";
        return html.str();
    }

    void DeliveryDossierBuilder::SaveIndexHtml(const DeliveryDossierManifest& manifest, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write delivery dossier index: " + filePath.string());
        }
        stream << BuildIndexHtml(manifest);
    }
}
