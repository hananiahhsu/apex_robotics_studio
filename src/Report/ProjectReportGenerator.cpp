#include "apex/report/ProjectReportGenerator.h"

#include "apex/core/MathTypes.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace apex::report
{
    namespace
    {
        std::string EscapeHtml(const std::string& text)
        {
            std::string value = text;
            auto replaceAll = [&](const std::string& from, const std::string& to)
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

    std::string ProjectReportGenerator::BuildHtmlReport(
        const apex::io::StudioProject& project,
        const apex::planning::TrajectoryPlan& plan,
        const apex::analysis::TrajectoryQualityReport& qualityReport,
        const std::string& embeddedSvg) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
             << "  <meta charset=\"utf-8\"/>\n"
             << "  <title>Apex Robotics Studio Report</title>\n"
             << "  <style>body{font-family:Arial,sans-serif;background:#0f172a;color:#e2e8f0;margin:0;padding:32px;}"
             << ".grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:16px;margin-bottom:24px;}"
             << ".card{background:#111827;border:1px solid #334155;border-radius:12px;padding:16px;}"
             << "table{border-collapse:collapse;width:100%;margin-top:12px;}th,td{border-bottom:1px solid #334155;padding:10px;text-align:left;}"
             << "h1,h2{margin:0 0 12px 0;} .warn{color:#fbbf24;} .bad{color:#f87171;} .good{color:#4ade80;} svg{max-width:100%;height:auto;border-radius:12px;}</style>\n"
             << "</head>\n<body>\n";
        html << "<h1>Apex Robotics Studio - Workcell Analysis Report</h1>\n";
        html << "<p>Project: <strong>" << EscapeHtml(project.ProjectName) << "</strong> | Robot: <strong>" << EscapeHtml(project.RobotName) << "</strong></p>\n";
        html << "<div class=\"grid\">\n";
        html << "  <div class=\"card\"><h2>Trajectory</h2><div>Samples: " << plan.Samples.size() << "</div><div>Cycle time: " << qualityReport.EstimatedCycleTimeSeconds << " s</div><div>Path length: " << qualityReport.PathLengthMeters << " m</div></div>\n";
        html << "  <div class=\"card\"><h2>Quality</h2><div class=\"" << (qualityReport.CollisionFree ? "good" : "bad") << "\">Collision free: " << (qualityReport.CollisionFree ? "YES" : "NO") << "</div><div>Collision samples: " << qualityReport.CollisionSampleCount << "</div><div>Peak TCP speed: " << qualityReport.PeakTcpSpeedMetersPerSecond << " m/s</div></div>\n";
        html << "  <div class=\"card\"><h2>Project</h2><div>Joints: " << project.Joints.size() << "</div><div>Obstacles: " << project.Obstacles.size() << "</div><div>Safety radius: " << project.Motion.LinkSafetyRadius << " m</div></div>\n";
        html << "</div>\n";
        html << "<div class=\"card\"><h2>Top View</h2>" << embeddedSvg << "</div>\n";
        html << "<div class=\"card\"><h2>Joint Velocity Metrics</h2><table><thead><tr><th>Joint</th><th>Max |vel| (rad/s)</th><th>Preferred limit</th><th>Status</th></tr></thead><tbody>\n";
        for (const auto& metric : qualityReport.JointVelocityMetrics)
        {
            html << "<tr><td>" << EscapeHtml(metric.JointName) << "</td><td>" << metric.MaxAbsVelocityRadPerSec << "</td><td>" << metric.PreferredVelocityLimitRadPerSec
                 << "</td><td class=\"" << (metric.ExceedsPreferredVelocityLimit ? "warn" : "good") << "\">" << (metric.ExceedsPreferredVelocityLimit ? "Review" : "OK") << "</td></tr>\n";
        }
        html << "</tbody></table></div>\n";
        html << "<div class=\"card\"><h2>Warnings</h2><ul>\n";
        if (qualityReport.Warnings.empty())
        {
            html << "<li class=\"good\">No warnings.</li>\n";
        }
        else
        {
            for (const auto& warning : qualityReport.Warnings)
            {
                html << "<li class=\"warn\">" << EscapeHtml(warning) << "</li>\n";
            }
        }
        html << "</ul></div>\n";
        html << "</body>\n</html>\n";
        return html.str();
    }

    std::string ProjectReportGenerator::BuildJobHtmlReport(
        const apex::io::StudioProject& project,
        const apex::simulation::JobSimulationResult& simulationResult,
        const std::vector<apex::extension::PluginFinding>& findings,
        const std::string& embeddedSvg) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
             << "  <meta charset=\"utf-8\"/>\n"
             << "  <title>Apex Robotics Studio Job Report</title>\n"
             << "  <style>body{font-family:Arial,sans-serif;background:#0b1220;color:#dbeafe;margin:0;padding:28px;}"
             << ".grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:16px;margin-bottom:24px;}"
             << ".card{background:#111827;border:1px solid #334155;border-radius:14px;padding:16px;}"
             << "table{border-collapse:collapse;width:100%;margin-top:12px;}th,td{border-bottom:1px solid #334155;padding:10px;text-align:left;vertical-align:top;}"
             << "h1,h2{margin:0 0 12px 0;} .warn{color:#fbbf24;} .bad{color:#f87171;} .good{color:#4ade80;} .info{color:#93c5fd;} svg{max-width:100%;height:auto;border-radius:12px;}</style>\n"
             << "</head>\n<body>\n";
        html << "<h1>Apex Robotics Studio - Production Job Report</h1>\n";
        html << "<p>Project: <strong>" << EscapeHtml(project.ProjectName) << "</strong> | Job: <strong>" << EscapeHtml(simulationResult.JobName) << "</strong> | Robot: <strong>" << EscapeHtml(project.RobotName) << "</strong></p>\n";
        html << "<div class=\"grid\">\n";
        html << "  <div class=\"card\"><h2>Execution</h2><div>Segments: " << simulationResult.SegmentResults.size() << "</div><div>Total duration: " << simulationResult.TotalDurationSeconds << " s</div></div>\n";
        html << "  <div class=\"card\"><h2>Motion</h2><div>Total samples: " << simulationResult.TotalSamples << "</div><div>Path length: " << simulationResult.TotalPathLengthMeters << " m</div></div>\n";
        html << "  <div class=\"card\"><h2>Safety</h2><div class=\"" << (simulationResult.CollisionFree ? "good" : "bad") << "\">Collision free: " << (simulationResult.CollisionFree ? "YES" : "NO") << "</div><div>Obstacles: " << project.Obstacles.size() << "</div></div>\n";
        html << "  <div class=\"card\"><h2>Structure</h2><div>Waypoints: " << project.Job.Waypoints.size() << "</div><div>Plugins: " << findings.size() << " findings</div></div>\n";
        html << "</div>\n";

        html << "<div class=\"card\"><h2>Top View</h2>" << embeddedSvg << "</div>\n";

        html << "<div class=\"card\"><h2>Segments</h2><table><thead><tr><th>Segment</th><th>Route</th><th>Tag</th><th>Duration</th><th>Path length</th><th>Peak TCP speed</th><th>Status</th></tr></thead><tbody>\n";
        for (const auto& segment : simulationResult.SegmentResults)
        {
            html << "<tr><td>" << EscapeHtml(segment.Segment.Name) << "</td><td>" << EscapeHtml(segment.Segment.StartWaypointName) << " → " << EscapeHtml(segment.Segment.GoalWaypointName)
                 << "</td><td>" << EscapeHtml(segment.Segment.ProcessTag) << "</td><td>" << segment.Quality.EstimatedCycleTimeSeconds
                 << " s</td><td>" << segment.Quality.PathLengthMeters << " m</td><td>" << segment.Quality.PeakTcpSpeedMetersPerSecond
                 << " m/s</td><td class=\"" << (segment.Quality.CollisionFree ? "good" : "bad") << "\">" << (segment.Quality.CollisionFree ? "OK" : "Collision") << "</td></tr>\n";
        }
        html << "</tbody></table></div>\n";

        html << "<div class=\"card\"><h2>Plugin Findings</h2><ul>\n";
        if (findings.empty())
        {
            html << "<li class=\"good\">No plugin findings.</li>\n";
        }
        else
        {
            for (const auto& finding : findings)
            {
                const std::string css = finding.Severity == apex::extension::FindingSeverity::Error ? "bad" :
                                        finding.Severity == apex::extension::FindingSeverity::Warning ? "warn" : "info";
                html << "<li class=\"" << css << "\">[" << EscapeHtml(apex::extension::ToString(finding.Severity)) << "] " << EscapeHtml(finding.Message) << "</li>\n";
            }
        }
        html << "</ul></div>\n";

        html << "<div class=\"card\"><h2>Execution Warnings</h2><ul>\n";
        if (simulationResult.Warnings.empty())
        {
            html << "<li class=\"good\">No execution warnings.</li>\n";
        }
        else
        {
            for (const auto& warning : simulationResult.Warnings)
            {
                html << "<li class=\"warn\">" << EscapeHtml(warning) << "</li>\n";
            }
        }
        html << "</ul></div>\n";

        html << "</body>\n</html>\n";
        return html.str();
    }

    void ProjectReportGenerator::SaveHtmlReport(
        const apex::io::StudioProject& project,
        const apex::planning::TrajectoryPlan& plan,
        const apex::analysis::TrajectoryQualityReport& qualityReport,
        const std::string& embeddedSvg,
        const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write HTML report: " + filePath.string());
        }
        stream << BuildHtmlReport(project, plan, qualityReport, embeddedSvg);
    }

    void ProjectReportGenerator::SaveJobHtmlReport(
        const apex::io::StudioProject& project,
        const apex::simulation::JobSimulationResult& simulationResult,
        const std::vector<apex::extension::PluginFinding>& findings,
        const std::string& embeddedSvg,
        const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write HTML job report: " + filePath.string());
        }
        stream << BuildJobHtmlReport(project, simulationResult, findings, embeddedSvg);
    }
}
