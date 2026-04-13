#include "apex/session/ExecutionSession.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace apex::session
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

    const char* ToString(SessionStatus status) noexcept
    {
        switch (status)
        {
        case SessionStatus::Success: return "success";
        case SessionStatus::Warning: return "warning";
        case SessionStatus::Failure: return "failure";
        }
        return "unknown";
    }

    void SessionRecorder::SetTitle(std::string title)
    {
        m_summary.Title = std::move(title);
    }

    void SessionRecorder::SetProjectName(std::string projectName)
    {
        m_summary.ProjectName = std::move(projectName);
    }

    void SessionRecorder::SetJobName(std::string jobName)
    {
        m_summary.JobName = std::move(jobName);
    }

    void SessionRecorder::SetMetrics(double totalDurationSeconds, double totalPathLengthMeters, std::size_t findingCount)
    {
        m_summary.TotalDurationSeconds = totalDurationSeconds;
        m_summary.TotalPathLengthMeters = totalPathLengthMeters;
        m_summary.FindingCount = findingCount;
    }

    void SessionRecorder::AddWarning(const std::string& warning)
    {
        m_summary.Warnings.push_back(warning);
    }

    void SessionRecorder::AddArtifact(std::string type, std::filesystem::path path, std::string description)
    {
        m_summary.Artifacts.push_back({std::move(type), std::move(path), std::move(description)});
    }

    void SessionRecorder::AddEvent(std::string category, std::string level, std::string message)
    {
        SessionEvent event;
        event.Sequence = static_cast<int>(m_summary.Events.size()) + 1;
        event.Category = std::move(category);
        event.Level = std::move(level);
        event.Message = std::move(message);
        m_summary.Events.push_back(std::move(event));
    }

    void SessionRecorder::MarkStatus(SessionStatus status) noexcept
    {
        m_summary.Status = status;
    }

    const ExecutionSessionSummary& SessionRecorder::GetSummary() const noexcept
    {
        return m_summary;
    }

    void SessionRecorder::SaveToDirectory(const std::filesystem::path& directory) const
    {
        std::filesystem::create_directories(directory);

        std::ofstream jsonStream(directory / "session_summary.json");
        if (!jsonStream)
        {
            throw std::runtime_error("Failed to write session summary JSON.");
        }

        jsonStream << "{\n"
                   << "  \"title\": \"" << EscapeJson(m_summary.Title) << "\",\n"
                   << "  \"project_name\": \"" << EscapeJson(m_summary.ProjectName) << "\",\n"
                   << "  \"job_name\": \"" << EscapeJson(m_summary.JobName) << "\",\n"
                   << "  \"status\": \"" << ToString(m_summary.Status) << "\",\n"
                   << "  \"total_duration_seconds\": " << m_summary.TotalDurationSeconds << ",\n"
                   << "  \"total_path_length_meters\": " << m_summary.TotalPathLengthMeters << ",\n"
                   << "  \"finding_count\": " << m_summary.FindingCount << ",\n"
                   << "  \"warnings\": [";
        for (std::size_t index = 0; index < m_summary.Warnings.size(); ++index)
        {
            if (index > 0)
            {
                jsonStream << ", ";
            }
            jsonStream << "\"" << EscapeJson(m_summary.Warnings[index]) << "\"";
        }
        jsonStream << "],\n  \"artifacts\": [\n";
        for (std::size_t index = 0; index < m_summary.Artifacts.size(); ++index)
        {
            const auto& artifact = m_summary.Artifacts[index];
            jsonStream << "    {\"type\": \"" << EscapeJson(artifact.Type) << "\", \"path\": \""
                       << EscapeJson(artifact.Path.generic_string()) << "\", \"description\": \""
                       << EscapeJson(artifact.Description) << "\"}";
            if (index + 1 < m_summary.Artifacts.size())
            {
                jsonStream << ',';
            }
            jsonStream << '\n';
        }
        jsonStream << "  ]\n}\n";

        std::ofstream jsonlStream(directory / "session_events.jsonl");
        if (!jsonlStream)
        {
            throw std::runtime_error("Failed to write session event JSONL.");
        }
        for (const auto& event : m_summary.Events)
        {
            jsonlStream << '{'
                        << "\"sequence\":" << event.Sequence << ','
                        << "\"category\":\"" << EscapeJson(event.Category) << "\"," 
                        << "\"level\":\"" << EscapeJson(event.Level) << "\"," 
                        << "\"message\":\"" << EscapeJson(event.Message) << "\""
                        << "}\n";
        }

        std::ofstream htmlStream(directory / "session_summary.html");
        if (!htmlStream)
        {
            throw std::runtime_error("Failed to write session summary HTML.");
        }
        htmlStream << BuildSummaryHtml();
    }

    std::string SessionRecorder::BuildSummaryHtml() const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"/>"
             << "<title>Apex Session Summary</title>"
             << "<style>body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;padding:24px;}"
             << ".card{background:#111827;border:1px solid #334155;border-radius:14px;padding:16px;margin-bottom:16px;}"
             << ".good{color:#4ade80;}.warn{color:#fbbf24;}.bad{color:#f87171;}table{width:100%;border-collapse:collapse;}th,td{padding:8px;border-bottom:1px solid #334155;text-align:left;}</style></head><body>";
        const char* statusCss = m_summary.Status == SessionStatus::Success ? "good" : m_summary.Status == SessionStatus::Warning ? "warn" : "bad";
        html << "<h1>Execution Session Summary</h1>";
        html << "<div class=\"card\"><div>Title: <strong>" << EscapeHtml(m_summary.Title) << "</strong></div>"
             << "<div>Project: <strong>" << EscapeHtml(m_summary.ProjectName) << "</strong></div>"
             << "<div>Job: <strong>" << EscapeHtml(m_summary.JobName) << "</strong></div>"
             << "<div class=\"" << statusCss << "\">Status: " << ToString(m_summary.Status) << "</div>"
             << "<div>Total duration: " << m_summary.TotalDurationSeconds << " s</div>"
             << "<div>Total path length: " << m_summary.TotalPathLengthMeters << " m</div>"
             << "<div>Findings: " << m_summary.FindingCount << "</div></div>";
        html << "<div class=\"card\"><h2>Warnings</h2><ul>";
        if (m_summary.Warnings.empty())
        {
            html << "<li class=\"good\">No warnings.</li>";
        }
        else
        {
            for (const auto& warning : m_summary.Warnings)
            {
                html << "<li class=\"warn\">" << EscapeHtml(warning) << "</li>";
            }
        }
        html << "</ul></div>";
        html << "<div class=\"card\"><h2>Artifacts</h2><table><thead><tr><th>Type</th><th>Path</th><th>Description</th></tr></thead><tbody>";
        for (const auto& artifact : m_summary.Artifacts)
        {
            html << "<tr><td>" << EscapeHtml(artifact.Type) << "</td><td>" << EscapeHtml(artifact.Path.generic_string()) << "</td><td>" << EscapeHtml(artifact.Description) << "</td></tr>";
        }
        html << "</tbody></table></div>";
        html << "<div class=\"card\"><h2>Events</h2><table><thead><tr><th>#</th><th>Category</th><th>Level</th><th>Message</th></tr></thead><tbody>";
        for (const auto& event : m_summary.Events)
        {
            html << "<tr><td>" << event.Sequence << "</td><td>" << EscapeHtml(event.Category) << "</td><td>" << EscapeHtml(event.Level) << "</td><td>" << EscapeHtml(event.Message) << "</td></tr>";
        }
        html << "</tbody></table></div></body></html>";
        return html.str();
    }
}
