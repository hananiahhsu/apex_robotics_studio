#include "apex/importer/RobotDescriptionInspector.h"

#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace apex::importer
{
    namespace
    {
        std::string EscapeJson(const std::string& value)
        {
            std::ostringstream builder;
            for (const char ch : value)
            {
                switch (ch)
                {
                case '\\': builder << "\\\\"; break;
                case '"': builder << "\\\""; break;
                case '\n': builder << "\\n"; break;
                case '\r': builder << "\\r"; break;
                case '\t': builder << "\\t"; break;
                default: builder << ch; break;
                }
            }
            return builder.str();
        }

        std::string EscapeHtml(const std::string& value)
        {
            std::string output;
            output.reserve(value.size());
            for (const char ch : value)
            {
                switch (ch)
                {
                case '&': output += "&amp;"; break;
                case '<': output += "&lt;"; break;
                case '>': output += "&gt;"; break;
                case '"': output += "&quot;"; break;
                default: output.push_back(ch); break;
                }
            }
            return output;
        }

        void WriteTextFile(const std::filesystem::path& filePath, const std::string& text)
        {
            if (!filePath.parent_path().empty())
            {
                std::filesystem::create_directories(filePath.parent_path());
            }
            std::ofstream stream(filePath);
            if (!stream)
            {
                throw std::runtime_error("Failed to write file: " + filePath.string());
            }
            stream << text;
        }
    }

    RobotDescriptionInspection RobotDescriptionInspector::Inspect(const apex::io::StudioProject& project) const
    {
        RobotDescriptionInspection inspection;
        inspection.ProjectName = project.ProjectName;
        inspection.RobotName = project.RobotName;
        inspection.SourceKind = project.DescriptionSource.SourceKind;
        inspection.SourcePath = project.DescriptionSource.SourcePath;
        inspection.ExpandedDescriptionPath = project.DescriptionSource.ExpandedDescriptionPath;
        inspection.PackageDependencies = project.DescriptionSource.PackageDependencies;
        inspection.PackageDependencyCount = inspection.PackageDependencies.size();
        inspection.MeshResourceCount = project.MeshResources.size();

        std::map<std::string, std::size_t> uriCounts;
        for (const auto& mesh : project.MeshResources)
        {
            if (!mesh.Uri.empty())
            {
                uriCounts[mesh.Uri] += 1;
            }
            const bool resolvedExists = !mesh.ResolvedPath.empty() && std::filesystem::exists(mesh.ResolvedPath);
            if (resolvedExists)
            {
                inspection.ResolvedMeshCount += 1;
            }
            else
            {
                inspection.MissingMeshCount += 1;
                inspection.MissingMeshUris.push_back(mesh.Uri.empty() ? std::string("<empty-uri>") : mesh.Uri);
            }
        }

        for (const auto& [uri, count] : uriCounts)
        {
            if (count > 1)
            {
                inspection.DuplicateMeshUris.push_back(uri);
            }
        }

        if (inspection.SourceKind.empty() || inspection.SourceKind == "manual")
        {
            inspection.Notes.push_back("Robot description source is not linked to URDF/Xacro import provenance.");
        }
        if (inspection.PackageDependencyCount == 0)
        {
            inspection.Notes.push_back("No external description package dependencies were recorded.");
        }
        if (inspection.MeshResourceCount == 0)
        {
            inspection.Notes.push_back("No mesh resources were captured during robot description import.");
        }
        if (inspection.MissingMeshCount > 0)
        {
            inspection.Notes.push_back("One or more mesh resources could not be resolved on disk.");
        }
        return inspection;
    }

    void RobotDescriptionInspector::WriteJsonReport(const RobotDescriptionInspection& inspection, const std::filesystem::path& outputPath) const
    {
        std::ostringstream builder;
        builder << "{\n"
                << "  \"project_name\": \"" << EscapeJson(inspection.ProjectName) << "\",\n"
                << "  \"robot_name\": \"" << EscapeJson(inspection.RobotName) << "\",\n"
                << "  \"source_kind\": \"" << EscapeJson(inspection.SourceKind) << "\",\n"
                << "  \"source_path\": \"" << EscapeJson(inspection.SourcePath) << "\",\n"
                << "  \"expanded_description_path\": \"" << EscapeJson(inspection.ExpandedDescriptionPath) << "\",\n"
                << "  \"package_dependency_count\": " << inspection.PackageDependencyCount << ",\n"
                << "  \"mesh_resource_count\": " << inspection.MeshResourceCount << ",\n"
                << "  \"resolved_mesh_count\": " << inspection.ResolvedMeshCount << ",\n"
                << "  \"missing_mesh_count\": " << inspection.MissingMeshCount << ",\n"
                << "  \"package_dependencies\": [";
        for (std::size_t index = 0; index < inspection.PackageDependencies.size(); ++index)
        {
            if (index > 0)
            {
                builder << ", ";
            }
            builder << '"' << EscapeJson(inspection.PackageDependencies[index]) << '"';
        }
        builder << "],\n  \"missing_mesh_uris\": [";
        for (std::size_t index = 0; index < inspection.MissingMeshUris.size(); ++index)
        {
            if (index > 0)
            {
                builder << ", ";
            }
            builder << '"' << EscapeJson(inspection.MissingMeshUris[index]) << '"';
        }
        builder << "],\n  \"duplicate_mesh_uris\": [";
        for (std::size_t index = 0; index < inspection.DuplicateMeshUris.size(); ++index)
        {
            if (index > 0)
            {
                builder << ", ";
            }
            builder << '"' << EscapeJson(inspection.DuplicateMeshUris[index]) << '"';
        }
        builder << "],\n  \"notes\": [";
        for (std::size_t index = 0; index < inspection.Notes.size(); ++index)
        {
            if (index > 0)
            {
                builder << ", ";
            }
            builder << '"' << EscapeJson(inspection.Notes[index]) << '"';
        }
        builder << "]\n}\n";
        WriteTextFile(outputPath, builder.str());
    }

    void RobotDescriptionInspector::WriteHtmlReport(const RobotDescriptionInspection& inspection, const std::filesystem::path& outputPath) const
    {
        std::ostringstream builder;
        builder << "<html><head><meta charset=\"utf-8\"><title>Robot Description Inspection</title>"
                << "<style>body{font-family:Arial,sans-serif;margin:24px;}table{border-collapse:collapse;width:100%;}"
                << "th,td{border:1px solid #ccc;padding:8px;text-align:left;}th{background:#f4f4f4;}"
                << "h1,h2{margin-bottom:8px;}ul{margin-top:4px;}code{background:#f5f5f5;padding:2px 4px;}</style></head><body>";
        builder << "<h1>Robot Description Inspection</h1>"
                << "<p><strong>Project:</strong> " << EscapeHtml(inspection.ProjectName)
                << "<br><strong>Robot:</strong> " << EscapeHtml(inspection.RobotName)
                << "<br><strong>Source kind:</strong> " << EscapeHtml(inspection.SourceKind)
                << "<br><strong>Source path:</strong> <code>" << EscapeHtml(inspection.SourcePath) << "</code>"
                << "<br><strong>Expanded description:</strong> <code>" << EscapeHtml(inspection.ExpandedDescriptionPath) << "</code></p>";
        builder << "<h2>Summary</h2><table><tr><th>Metric</th><th>Value</th></tr>"
                << "<tr><td>Package dependencies</td><td>" << inspection.PackageDependencyCount << "</td></tr>"
                << "<tr><td>Mesh resources</td><td>" << inspection.MeshResourceCount << "</td></tr>"
                << "<tr><td>Resolved meshes</td><td>" << inspection.ResolvedMeshCount << "</td></tr>"
                << "<tr><td>Missing meshes</td><td>" << inspection.MissingMeshCount << "</td></tr></table>";

        const auto writeList = [&](const char* title, const std::vector<std::string>& items)
        {
            builder << "<h2>" << title << "</h2><ul>";
            if (items.empty())
            {
                builder << "<li>None</li>";
            }
            else
            {
                for (const auto& item : items)
                {
                    builder << "<li><code>" << EscapeHtml(item) << "</code></li>";
                }
            }
            builder << "</ul>";
        };

        writeList("Package Dependencies", inspection.PackageDependencies);
        writeList("Missing Mesh URIs", inspection.MissingMeshUris);
        writeList("Duplicate Mesh URIs", inspection.DuplicateMeshUris);
        writeList("Notes", inspection.Notes);
        builder << "</body></html>";
        WriteTextFile(outputPath, builder.str());
    }
}
