#include "apex/trace/AuditTrail.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace apex::trace
{
    namespace
    {
        std::string EscapeJson(const std::string& text)
        {
            std::ostringstream builder;
            for (const char ch : text)
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

    void AuditTrailRecorder::SetTitle(std::string title)
    {
        m_summary.Title = std::move(title);
    }

    void AuditTrailRecorder::SetProjectName(std::string projectName)
    {
        m_summary.ProjectName = std::move(projectName);
    }

    void AuditTrailRecorder::SetProjectFingerprint(std::string fingerprint)
    {
        m_summary.ProjectFingerprint = std::move(fingerprint);
    }

    void AuditTrailRecorder::SetOverallStatus(std::string overallStatus)
    {
        m_summary.OverallStatus = std::move(overallStatus);
    }

    void AuditTrailRecorder::AddArtifact(std::string type, std::filesystem::path relativePath, std::string description)
    {
        m_summary.Artifacts.push_back({std::move(type), std::move(relativePath), std::move(description)});
    }

    void AuditTrailRecorder::AddEvent(std::string category, std::string level, std::string action, std::string message)
    {
        AuditEvent event;
        event.Sequence = static_cast<int>(m_summary.Events.size()) + 1;
        event.Category = std::move(category);
        event.Level = std::move(level);
        event.Action = std::move(action);
        event.Message = std::move(message);
        m_summary.Events.push_back(std::move(event));
    }

    const AuditTrailSummary& AuditTrailRecorder::GetSummary() const noexcept
    {
        return m_summary;
    }

    void AuditTrailRecorder::SaveToDirectory(const std::filesystem::path& directory) const
    {
        std::filesystem::create_directories(directory);

        std::ofstream summary(directory / "audit_summary.json");
        if (!summary)
        {
            throw std::runtime_error("Failed to write audit summary JSON.");
        }
        summary << "{\n"
                << "  \"title\": \"" << EscapeJson(m_summary.Title) << "\",\n"
                << "  \"project_name\": \"" << EscapeJson(m_summary.ProjectName) << "\",\n"
                << "  \"project_fingerprint\": \"" << EscapeJson(m_summary.ProjectFingerprint) << "\",\n"
                << "  \"overall_status\": \"" << EscapeJson(m_summary.OverallStatus) << "\",\n"
                << "  \"artifacts\": [\n";
        for (std::size_t index = 0; index < m_summary.Artifacts.size(); ++index)
        {
            const auto& artifact = m_summary.Artifacts[index];
            summary << "    {\"type\": \"" << EscapeJson(artifact.Type) << "\", \"path\": \""
                    << EscapeJson(artifact.RelativePath.generic_string()) << "\", \"description\": \""
                    << EscapeJson(artifact.Description) << "\"}";
            if (index + 1 < m_summary.Artifacts.size())
            {
                summary << ',';
            }
            summary << '\n';
        }
        summary << "  ]\n}\n";

        std::ofstream events(directory / "audit_events.jsonl");
        if (!events)
        {
            throw std::runtime_error("Failed to write audit event JSONL.");
        }
        for (const auto& event : m_summary.Events)
        {
            events << '{'
                   << "\"sequence\":" << event.Sequence << ','
                   << "\"category\":\"" << EscapeJson(event.Category) << "\"," 
                   << "\"level\":\"" << EscapeJson(event.Level) << "\"," 
                   << "\"action\":\"" << EscapeJson(event.Action) << "\"," 
                   << "\"message\":\"" << EscapeJson(event.Message) << "\""
                   << "}\n";
        }

        std::ofstream html(directory / "audit_index.html");
        if (!html)
        {
            throw std::runtime_error("Failed to write audit HTML index.");
        }
        html << BuildHtmlIndex();
    }

    std::string AuditTrailRecorder::BuildHtmlIndex() const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"/>"
             << "<title>Apex Audit Trail</title>"
             << "<style>body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;padding:24px;}"
             << ".card{background:#111827;border:1px solid #334155;border-radius:14px;padding:16px;margin-bottom:16px;}"
             << "table{width:100%;border-collapse:collapse;}th,td{padding:8px;border-bottom:1px solid #334155;text-align:left;}"
             << ".success{color:#4ade80;}.warning{color:#fbbf24;}.error{color:#f87171;}</style></head><body>";
        html << "<h1>Audit Trail</h1>";
        html << "<div class=\"card\"><div>Title: <strong>" << EscapeHtml(m_summary.Title) << "</strong></div>"
             << "<div>Project: <strong>" << EscapeHtml(m_summary.ProjectName) << "</strong></div>"
             << "<div>Fingerprint: <strong>" << EscapeHtml(m_summary.ProjectFingerprint) << "</strong></div>"
             << "<div>Status: <strong class=\"" << EscapeHtml(m_summary.OverallStatus) << "\">" << EscapeHtml(m_summary.OverallStatus) << "</strong></div></div>";
        html << "<div class=\"card\"><h2>Artifacts</h2><table><thead><tr><th>Type</th><th>Path</th><th>Description</th></tr></thead><tbody>";
        for (const auto& artifact : m_summary.Artifacts)
        {
            html << "<tr><td>" << EscapeHtml(artifact.Type) << "</td><td>" << EscapeHtml(artifact.RelativePath.generic_string()) << "</td><td>" << EscapeHtml(artifact.Description) << "</td></tr>";
        }
        html << "</tbody></table></div>";
        html << "<div class=\"card\"><h2>Events</h2><table><thead><tr><th>#</th><th>Category</th><th>Level</th><th>Action</th><th>Message</th></tr></thead><tbody>";
        for (const auto& event : m_summary.Events)
        {
            html << "<tr><td>" << event.Sequence << "</td><td>" << EscapeHtml(event.Category) << "</td><td>" << EscapeHtml(event.Level)
                 << "</td><td>" << EscapeHtml(event.Action) << "</td><td>" << EscapeHtml(event.Message) << "</td></tr>";
        }
        html << "</tbody></table></div></body></html>";
        return html.str();
    }
}
