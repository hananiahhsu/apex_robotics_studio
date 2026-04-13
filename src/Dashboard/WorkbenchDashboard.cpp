#include "apex/dashboard/WorkbenchDashboard.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace apex::dashboard
{
    namespace
    {
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

        const char* SeverityCss(apex::extension::FindingSeverity severity)
        {
            switch (severity)
            {
            case apex::extension::FindingSeverity::Info: return "info";
            case apex::extension::FindingSeverity::Warning: return "warn";
            case apex::extension::FindingSeverity::Error: return "bad";
            }
            return "info";
        }
    }

    std::string WorkbenchDashboardGenerator::BuildProjectDashboard(
        const apex::io::StudioProject& project,
        const apex::catalog::ProjectCatalog& catalog,
        const apex::simulation::JobSimulationResult& simulationResult,
        const std::vector<apex::extension::PluginFinding>& findings,
        const apex::session::ExecutionSessionSummary& sessionSummary,
        const std::string& embeddedSvg) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"/>"
             << "<title>Apex Robotics Workstation Dashboard</title>"
             << "<style>body{font-family:Arial,sans-serif;background:#0b1220;color:#e5eefb;margin:0;}"
             << ".shell{display:grid;grid-template-columns:280px 1fr;min-height:100vh;}"
             << ".nav{background:#111827;border-right:1px solid #334155;padding:20px;position:sticky;top:0;height:100vh;overflow:auto;}"
             << ".nav a{display:block;color:#93c5fd;text-decoration:none;padding:8px 0;}"
             << ".content{padding:24px;}"
             << ".grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:16px;margin-bottom:20px;}"
             << ".card{background:#111827;border:1px solid #334155;border-radius:14px;padding:16px;margin-bottom:18px;}"
             << "table{border-collapse:collapse;width:100%;}th,td{border-bottom:1px solid #334155;padding:10px;text-align:left;vertical-align:top;}"
             << ".good{color:#4ade80;}.warn{color:#fbbf24;}.bad{color:#f87171;}.info{color:#93c5fd;}"
             << "svg{max-width:100%;height:auto;border-radius:12px;}</style></head><body>";

        html << "<div class=\"shell\"><aside class=\"nav\"><h2>Workbench</h2>"
             << "<div><strong>Project</strong><br/>" << EscapeHtml(project.ProjectName) << "</div><br/>"
             << "<a href=\"#overview\">Overview</a><a href=\"#catalog\">Project Catalog</a><a href=\"#layout\">Workcell Layout</a>"
             << "<a href=\"#segments\">Segments</a><a href=\"#findings\">Findings</a><a href=\"#session\">Execution Session</a></aside><main class=\"content\">";

        html << "<section id=\"overview\"><h1>Apex Robotics Workstation Dashboard</h1>"
             << "<p>Robot: <strong>" << EscapeHtml(project.RobotName) << "</strong> | Job: <strong>" << EscapeHtml(simulationResult.JobName) << "</strong></p>"
             << "<div class=\"grid\">"
             << "<div class=\"card\"><h3>Joints</h3><div>" << catalog.JointCount << "</div></div>"
             << "<div class=\"card\"><h3>Obstacles</h3><div>" << catalog.ObstacleCount << "</div></div>"
             << "<div class=\"card\"><h3>Segments</h3><div>" << simulationResult.SegmentResults.size() << "</div></div>"
             << "<div class=\"card\"><h3>Status</h3><div class=\"" << (simulationResult.CollisionFree ? "good" : "warn") << "\">"
             << (simulationResult.CollisionFree ? "Ready" : "Review") << "</div></div></div></section>";

        html << "<div class=\"grid\">"
             << "<div class=\"card\"><h3>Cycle Time</h3><div>" << simulationResult.TotalDurationSeconds << " s</div></div>"
             << "<div class=\"card\"><h3>Path Length</h3><div>" << simulationResult.TotalPathLengthMeters << " m</div></div>"
             << "<div class=\"card\"><h3>Findings</h3><div>" << findings.size() << "</div></div>"
             << "<div class=\"card\"><h3>Warnings</h3><div>" << sessionSummary.Warnings.size() << "</div></div></div>";

        html << "<section id=\"catalog\" class=\"card\"><h2>Project Catalog</h2><table><thead><tr><th>Kind</th><th>Name</th><th>Details</th></tr></thead><tbody>";
        for (const auto& item : catalog.Items)
        {
            html << "<tr><td>" << EscapeHtml(item.Kind) << "</td><td>"
                 << EscapeHtml(std::string(static_cast<std::size_t>(item.Depth) * 2, ' ') + item.Name)
                 << "</td><td>" << EscapeHtml(item.Details) << "</td></tr>";
        }
        html << "</tbody></table></section>";

        html << "<section id=\"layout\" class=\"card\"><h2>Workcell Layout</h2>" << embeddedSvg << "</section>";

        html << "<section id=\"segments\" class=\"card\"><h2>Segment Execution</h2><table><thead><tr><th>Segment</th><th>Route</th><th>Tag</th><th>Duration</th><th>Path</th><th>Peak TCP</th><th>Status</th></tr></thead><tbody>";
        for (const auto& segment : simulationResult.SegmentResults)
        {
            html << "<tr><td>" << EscapeHtml(segment.Segment.Name) << "</td><td>" << EscapeHtml(segment.Segment.StartWaypointName)
                 << " -> " << EscapeHtml(segment.Segment.GoalWaypointName) << "</td><td>" << EscapeHtml(segment.Segment.ProcessTag)
                 << "</td><td>" << segment.Quality.EstimatedCycleTimeSeconds << " s</td><td>" << segment.Quality.PathLengthMeters
                 << " m</td><td>" << segment.Quality.PeakTcpSpeedMetersPerSecond << " m/s</td><td class=\""
                 << (segment.Quality.CollisionFree ? "good" : "bad") << "\">" << (segment.Quality.CollisionFree ? "OK" : "Collision") << "</td></tr>";
        }
        html << "</tbody></table></section>";

        html << "<section id=\"findings\" class=\"card\"><h2>Audit Findings</h2><ul>";
        if (findings.empty())
        {
            html << "<li class=\"good\">No findings.</li>";
        }
        else
        {
            for (const auto& finding : findings)
            {
                html << "<li class=\"" << SeverityCss(finding.Severity) << "\">["
                     << EscapeHtml(apex::extension::ToString(finding.Severity)) << "] "
                     << EscapeHtml(finding.Message) << "</li>";
            }
        }
        html << "</ul></section>";

        html << "<section id=\"session\" class=\"card\"><h2>Execution Session</h2>"
             << "<div>Status: <strong class=\""
             << (sessionSummary.Status == apex::session::SessionStatus::Success ? "good" : sessionSummary.Status == apex::session::SessionStatus::Warning ? "warn" : "bad")
             << "\">" << EscapeHtml(apex::session::ToString(sessionSummary.Status)) << "</strong></div>"
             << "<div>Artifacts: " << sessionSummary.Artifacts.size() << "</div>"
             << "<table><thead><tr><th>#</th><th>Category</th><th>Level</th><th>Message</th></tr></thead><tbody>";
        for (const auto& event : sessionSummary.Events)
        {
            html << "<tr><td>" << event.Sequence << "</td><td>" << EscapeHtml(event.Category) << "</td><td>" << EscapeHtml(event.Level) << "</td><td>" << EscapeHtml(event.Message) << "</td></tr>";
        }
        html << "</tbody></table></section>";

        html << "</main></div></body></html>";
        return html.str();
    }

    std::string WorkbenchDashboardGenerator::BuildBatchDashboard(
        const apex::ops::BatchRunResult& batchResult,
        const apex::session::ExecutionSessionSummary& sessionSummary) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"utf-8\"/>"
             << "<title>Apex Batch Dashboard</title>"
             << "<style>body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;padding:24px;}"
             << ".grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:16px;margin-bottom:18px;}"
             << ".card{background:#111827;border:1px solid #334155;border-radius:14px;padding:16px;margin-bottom:18px;}"
             << "table{border-collapse:collapse;width:100%;}th,td{padding:10px;border-bottom:1px solid #334155;text-align:left;}"
             << ".good{color:#4ade80;}.warn{color:#fbbf24;}.bad{color:#f87171;}</style></head><body>";
        html << "<h1>Apex Batch Validation Dashboard</h1><p>Batch: <strong>" << EscapeHtml(batchResult.Name) << "</strong></p>";
        html << "<div class=\"grid\">"
             << "<div class=\"card\"><h3>Total Entries</h3><div>" << batchResult.Entries.size() << "</div></div>"
             << "<div class=\"card\"><h3>Success</h3><div class=\"good\">" << batchResult.SuccessCount << "</div></div>"
             << "<div class=\"card\"><h3>Failure</h3><div class=\"bad\">" << batchResult.FailureCount << "</div></div>"
             << "<div class=\"card\"><h3>Session Status</h3><div>" << EscapeHtml(apex::session::ToString(sessionSummary.Status)) << "</div></div></div>";
        html << "<div class=\"card\"><h2>Batch Entries</h2><table><thead><tr><th>Name</th><th>Status</th><th>Collision Free</th><th>Duration</th><th>Path Length</th><th>Findings</th></tr></thead><tbody>";
        for (const auto& entry : batchResult.Entries)
        {
            html << "<tr><td>" << EscapeHtml(entry.Entry.Name) << "</td><td class=\"" << (entry.Success ? "good" : "bad") << "\">" << (entry.Success ? "OK" : "Failed")
                 << "</td><td class=\"" << (entry.CollisionFree ? "good" : "warn") << "\">" << (entry.CollisionFree ? "YES" : "NO")
                 << "</td><td>" << entry.TotalDurationSeconds << " s</td><td>" << entry.TotalPathLengthMeters << " m</td><td>" << entry.FindingCount << "</td></tr>";
        }
        html << "</tbody></table></div>";
        html << "<div class=\"card\"><h2>Session Events</h2><table><thead><tr><th>#</th><th>Category</th><th>Level</th><th>Message</th></tr></thead><tbody>";
        for (const auto& event : sessionSummary.Events)
        {
            html << "<tr><td>" << event.Sequence << "</td><td>" << EscapeHtml(event.Category) << "</td><td>" << EscapeHtml(event.Level) << "</td><td>" << EscapeHtml(event.Message) << "</td></tr>";
        }
        html << "</tbody></table></div></body></html>";
        return html.str();
    }

    void WorkbenchDashboardGenerator::SaveProjectDashboard(
        const apex::io::StudioProject& project,
        const apex::catalog::ProjectCatalog& catalog,
        const apex::simulation::JobSimulationResult& simulationResult,
        const std::vector<apex::extension::PluginFinding>& findings,
        const apex::session::ExecutionSessionSummary& sessionSummary,
        const std::string& embeddedSvg,
        const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write project dashboard: " + filePath.string());
        }
        stream << BuildProjectDashboard(project, catalog, simulationResult, findings, sessionSummary, embeddedSvg);
    }

    void WorkbenchDashboardGenerator::SaveBatchDashboard(
        const apex::ops::BatchRunResult& batchResult,
        const apex::session::ExecutionSessionSummary& sessionSummary,
        const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write batch dashboard: " + filePath.string());
        }
        stream << BuildBatchDashboard(batchResult, sessionSummary);
    }
}
