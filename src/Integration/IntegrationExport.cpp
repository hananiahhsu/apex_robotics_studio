#include "apex/integration/IntegrationExport.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace apex::integration
{
    namespace
    {
        std::string NowUtcString()
        {
            const auto now = std::chrono::system_clock::now();
            const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
            std::tm timeInfo{};
        #if defined(_WIN32)
            gmtime_s(&timeInfo, &nowTime);
        #else
            gmtime_r(&nowTime, &timeInfo);
        #endif
            std::ostringstream builder;
            builder << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%SZ");
            return builder.str();
        }

        std::string EscapeJson(const std::string& value)
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

    IntegrationExportSummary IntegrationPackageExporter::Export(
        const apex::io::StudioProject& project,
        const apex::simulation::JobSimulationResult& simulation,
        const apex::quality::GateEvaluation& gate,
        const std::vector<apex::extension::PluginFinding>& findings,
        const std::filesystem::path& outputDirectory,
        const apex::governance::ProjectApprovalVerification* approvalVerification) const
    {
        std::filesystem::create_directories(outputDirectory);

        IntegrationExportSummary summary;
        summary.ProjectName = project.ProjectName;
        summary.ProjectFingerprint = apex::governance::ProjectFingerprintBuilder().Build(project);
        summary.ExportedAtUtc = NowUtcString();
        summary.GateStatus = gate.Status;
        summary.ApprovalDecision = approvalVerification != nullptr ? approvalVerification->OverallDecision : apex::governance::ApprovalDecision::Pending;
        summary.CollisionFree = simulation.CollisionFree;
        summary.SegmentCount = simulation.SegmentResults.size();
        summary.ObstacleCount = project.Obstacles.size();
        summary.FindingCount = findings.size();
        summary.TotalDurationSeconds = simulation.TotalDurationSeconds;
        summary.TotalPathLengthMeters = simulation.TotalPathLengthMeters;

        {
            std::ofstream stream(outputDirectory / "summary.json");
            if (!stream)
            {
                throw std::runtime_error("Failed to write integration summary JSON.");
            }
            stream << "{\n"
                   << "  \"project_name\": \"" << EscapeJson(summary.ProjectName) << "\",\n"
                   << "  \"project_fingerprint\": \"" << EscapeJson(summary.ProjectFingerprint) << "\",\n"
                   << "  \"exported_at_utc\": \"" << EscapeJson(summary.ExportedAtUtc) << "\",\n"
                   << "  \"gate_status\": \"" << apex::quality::ToString(summary.GateStatus) << "\",\n"
                   << "  \"approval_decision\": \"" << apex::governance::ToString(summary.ApprovalDecision) << "\",\n"
                   << "  \"collision_free\": " << (summary.CollisionFree ? "true" : "false") << ",\n"
                   << "  \"segment_count\": " << summary.SegmentCount << ",\n"
                   << "  \"obstacle_count\": " << summary.ObstacleCount << ",\n"
                   << "  \"finding_count\": " << summary.FindingCount << ",\n"
                   << "  \"total_duration_seconds\": " << summary.TotalDurationSeconds << ",\n"
                   << "  \"total_path_length_meters\": " << summary.TotalPathLengthMeters << "\n"
                   << "}\n";
        }

        {
            std::ofstream stream(outputDirectory / "segments.csv");
            if (!stream)
            {
                throw std::runtime_error("Failed to write integration segments CSV.");
            }
            stream << "segment_name,process_tag,duration_seconds,path_length_meters,collision_free\n";
            for (const auto& segment : simulation.SegmentResults)
            {
                stream << segment.Segment.Name << ',' << segment.Segment.ProcessTag << ','
                       << segment.Quality.EstimatedCycleTimeSeconds << ',' << segment.Quality.PathLengthMeters << ','
                       << (segment.Quality.CollisionFree ? 1 : 0) << '\n';
            }
        }

        {
            std::ofstream stream(outputDirectory / "findings.csv");
            if (!stream)
            {
                throw std::runtime_error("Failed to write integration findings CSV.");
            }
            stream << "severity,message\n";
            for (const auto& finding : findings)
            {
                stream << apex::extension::ToString(finding.Severity) << ',' << '"' << finding.Message << '"' << '\n';
            }
        }

        {
            std::ofstream stream(outputDirectory / "obstacles.csv");
            if (!stream)
            {
                throw std::runtime_error("Failed to write integration obstacles CSV.");
            }
            stream << "name,min_x,min_y,min_z,max_x,max_y,max_z\n";
            for (const auto& obstacle : project.Obstacles)
            {
                stream << obstacle.Name << ',' << obstacle.Bounds.Min.X << ',' << obstacle.Bounds.Min.Y << ',' << obstacle.Bounds.Min.Z << ','
                       << obstacle.Bounds.Max.X << ',' << obstacle.Bounds.Max.Y << ',' << obstacle.Bounds.Max.Z << '\n';
            }
        }

        {
            std::ofstream stream(outputDirectory / "README.txt");
            if (!stream)
            {
                throw std::runtime_error("Failed to write integration README.");
            }
            stream << "ApexRoboticsStudio integration package\n"
                   << "Project: " << summary.ProjectName << "\n"
                   << "Fingerprint: " << summary.ProjectFingerprint << "\n"
                   << "Gate: " << apex::quality::ToString(summary.GateStatus) << "\n"
                   << "Approval: " << apex::governance::ToString(summary.ApprovalDecision) << "\n"
                   << "Artifacts:\n  - summary.json\n  - segments.csv\n  - findings.csv\n  - obstacles.csv\n";
        }

        return summary;
    }
}
