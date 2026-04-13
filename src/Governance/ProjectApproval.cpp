#include "apex/governance/ProjectApproval.h"

#include "apex/core/MathTypes.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace apex::governance
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
                    throw std::invalid_argument("Approval file contains key-value pair before any section.");
                }
                const std::size_t separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    throw std::invalid_argument("Approval file line is not key=value: " + trimmed);
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

        std::string EscapeHtml(const std::string& value)
        {
            std::string out;
            out.reserve(value.size());
            for (const char ch : value)
            {
                switch (ch)
                {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                default: out += ch; break;
                }
            }
            return out;
        }

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

        void HashCombine(std::uint64_t& hash, const std::string& value)
        {
            constexpr std::uint64_t prime = 1099511628211ull;
            for (const unsigned char ch : value)
            {
                hash ^= static_cast<std::uint64_t>(ch);
                hash *= prime;
            }
            hash ^= static_cast<std::uint64_t>('|');
            hash *= prime;
        }

        void HashCombine(std::uint64_t& hash, double value)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(9);
            builder << value;
            HashCombine(hash, builder.str());
        }
    }

    std::string ToString(ApprovalDecision decision) noexcept
    {
        switch (decision)
        {
        case ApprovalDecision::Pending: return "pending";
        case ApprovalDecision::Approved: return "approved";
        case ApprovalDecision::Rejected: return "rejected";
        }
        return "pending";
    }

    ApprovalDecision ApprovalDecisionFromString(const std::string& text)
    {
        const std::string normalized = Trim(text);
        if (normalized == "pending")
        {
            return ApprovalDecision::Pending;
        }
        if (normalized == "approved" || normalized == "approve" || normalized == "pass")
        {
            return ApprovalDecision::Approved;
        }
        if (normalized == "rejected" || normalized == "reject" || normalized == "fail")
        {
            return ApprovalDecision::Rejected;
        }
        throw std::invalid_argument("Unknown approval decision: " + text);
    }

    std::string ProjectFingerprintBuilder::Build(const apex::io::StudioProject& project) const
    {
        std::uint64_t hash = 1469598103934665603ull;
        HashCombine(hash, std::to_string(project.SchemaVersion));
        HashCombine(hash, project.ProjectName);
        HashCombine(hash, project.RobotName);
        HashCombine(hash, project.Metadata.CellName);
        HashCombine(hash, project.Metadata.ProcessFamily);
        HashCombine(hash, project.Metadata.Owner);
        for (const auto& joint : project.Joints)
        {
            HashCombine(hash, joint.Name);
            HashCombine(hash, joint.LinkLength);
            HashCombine(hash, joint.MinAngleRad);
            HashCombine(hash, joint.MaxAngleRad);
        }
        for (const auto& obstacle : project.Obstacles)
        {
            HashCombine(hash, obstacle.Name);
            HashCombine(hash, obstacle.Bounds.Min.X);
            HashCombine(hash, obstacle.Bounds.Min.Y);
            HashCombine(hash, obstacle.Bounds.Min.Z);
            HashCombine(hash, obstacle.Bounds.Max.X);
            HashCombine(hash, obstacle.Bounds.Max.Y);
            HashCombine(hash, obstacle.Bounds.Max.Z);
        }
        HashCombine(hash, project.Motion.SampleCount == 0 ? std::string("0") : std::to_string(project.Motion.SampleCount));
        HashCombine(hash, project.Motion.DurationSeconds);
        HashCombine(hash, project.Motion.LinkSafetyRadius);
        for (double angle : project.Motion.Start.JointAnglesRad)
        {
            HashCombine(hash, angle);
        }
        for (double angle : project.Motion.Goal.JointAnglesRad)
        {
            HashCombine(hash, angle);
        }
        HashCombine(hash, project.Job.Name);
        for (const auto& waypoint : project.Job.Waypoints)
        {
            HashCombine(hash, waypoint.Name);
            for (double angle : waypoint.State.JointAnglesRad)
            {
                HashCombine(hash, angle);
            }
        }
        for (const auto& segment : project.Job.Segments)
        {
            HashCombine(hash, segment.Name);
            HashCombine(hash, segment.StartWaypointName);
            HashCombine(hash, segment.GoalWaypointName);
            HashCombine(hash, std::to_string(segment.SampleCount));
            HashCombine(hash, segment.DurationSeconds);
            HashCombine(hash, segment.ProcessTag);
        }

        std::ostringstream builder;
        builder << std::hex << std::setw(16) << std::setfill('0') << hash;
        return builder.str();
    }

    ProjectApprovalDocument ProjectApprovalSerializer::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open approval file: " + filePath.string());
        }

        const auto sections = ParseSections(stream);
        ProjectApprovalDocument document;
        for (const auto& section : sections)
        {
            if (section.Name == "approval")
            {
                document.SchemaVersion = ParseInt(GetRequired(section.Values, "schema_version", section.Name), "schema_version");
                document.Title = GetRequired(section.Values, "title", section.Name);
                document.ProjectName = GetRequired(section.Values, "project_name", section.Name);
            }
            else if (section.Name == "required_role")
            {
                document.RequiredRoles.push_back(GetRequired(section.Values, "name", section.Name));
            }
            else if (section.Name == "entry")
            {
                ApprovalEntry entry;
                entry.Role = GetRequired(section.Values, "role", section.Name);
                entry.Signer = GetRequired(section.Values, "signer", section.Name);
                entry.Decision = ApprovalDecisionFromString(GetRequired(section.Values, "decision", section.Name));
                entry.TimestampUtc = GetRequired(section.Values, "timestamp_utc", section.Name);
                entry.ProjectFingerprint = GetRequired(section.Values, "project_fingerprint", section.Name);
                const auto itNotes = section.Values.find("notes");
                if (itNotes != section.Values.end())
                {
                    entry.Notes = itNotes->second;
                }
                document.Entries.push_back(entry);
            }
        }
        return document;
    }

    void ProjectApprovalSerializer::SaveToFile(const ProjectApprovalDocument& document, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write approval file: " + filePath.string());
        }

        stream << "; ApexRoboticsStudio approval workflow\n"
               << "[approval]\n"
               << "schema_version=" << document.SchemaVersion << '\n'
               << "title=" << document.Title << '\n'
               << "project_name=" << document.ProjectName << "\n\n";
        for (const auto& role : document.RequiredRoles)
        {
            stream << "[required_role]\n"
                   << "name=" << role << "\n\n";
        }
        for (const auto& entry : document.Entries)
        {
            stream << "[entry]\n"
                   << "role=" << entry.Role << '\n'
                   << "signer=" << entry.Signer << '\n'
                   << "decision=" << ToString(entry.Decision) << '\n'
                   << "timestamp_utc=" << entry.TimestampUtc << '\n'
                   << "project_fingerprint=" << entry.ProjectFingerprint << '\n'
                   << "notes=" << entry.Notes << "\n\n";
        }
    }

    ProjectApprovalDocument ProjectApprovalWorkflow::CreateTemplate(const apex::io::StudioProject& project) const
    {
        ProjectApprovalDocument document;
        document.Title = project.ProjectName + " Approval Workflow";
        document.ProjectName = project.ProjectName;
        document.RequiredRoles = {"robotics_lead", "process_owner", "qa"};
        return document;
    }

    void ProjectApprovalWorkflow::Sign(
        const apex::io::StudioProject& project,
        ProjectApprovalDocument& document,
        const std::string& role,
        const std::string& signer,
        ApprovalDecision decision,
        const std::string& notes) const
    {
        if (std::find(document.RequiredRoles.begin(), document.RequiredRoles.end(), role) == document.RequiredRoles.end())
        {
            document.RequiredRoles.push_back(role);
        }
        ApprovalEntry entry;
        entry.Role = role;
        entry.Signer = signer;
        entry.Decision = decision;
        entry.Notes = notes;
        entry.TimestampUtc = NowUtcString();
        entry.ProjectFingerprint = ProjectFingerprintBuilder().Build(project);
        document.Entries.push_back(entry);
    }

    ProjectApprovalVerification ProjectApprovalWorkflow::Verify(
        const apex::io::StudioProject& project,
        const ProjectApprovalDocument& document) const
    {
        ProjectApprovalVerification verification;
        verification.ProjectFingerprint = ProjectFingerprintBuilder().Build(project);

        bool anyRejected = false;
        bool allApproved = true;
        for (const auto& role : document.RequiredRoles)
        {
            RoleApprovalStatus status;
            status.Role = role;
            for (auto it = document.Entries.rbegin(); it != document.Entries.rend(); ++it)
            {
                if (it->Role != role)
                {
                    continue;
                }
                status.LatestSigner = it->Signer;
                status.LatestTimestampUtc = it->TimestampUtc;
                status.LatestNotes = it->Notes;
                if (it->ProjectFingerprint == verification.ProjectFingerprint)
                {
                    if (it->Decision == ApprovalDecision::Approved)
                    {
                        status.IsSatisfied = true;
                        status.HasCurrentApproval = true;
                    }
                    else if (it->Decision == ApprovalDecision::Rejected)
                    {
                        status.HasCurrentRejection = true;
                    }
                }
                break;
            }

            if (status.HasCurrentRejection)
            {
                anyRejected = true;
                verification.Notes.push_back("Role '" + role + "' rejected the current project revision.");
            }
            else if (!status.IsSatisfied)
            {
                allApproved = false;
                verification.Notes.push_back("Role '" + role + "' does not have a current approval for fingerprint " + verification.ProjectFingerprint + ".");
            }
            verification.RoleStatuses.push_back(status);
        }

        if (anyRejected)
        {
            verification.OverallDecision = ApprovalDecision::Rejected;
        }
        else if (allApproved && !document.RequiredRoles.empty())
        {
            verification.OverallDecision = ApprovalDecision::Approved;
            verification.Notes.push_back("All required roles approved the current project revision.");
        }
        else
        {
            verification.OverallDecision = ApprovalDecision::Pending;
        }
        return verification;
    }

    std::string ProjectApprovalWorkflow::BuildHtmlReport(
        const apex::io::StudioProject& project,
        const ProjectApprovalDocument& document,
        const ProjectApprovalVerification& verification) const
    {
        std::ostringstream html;
        html << "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Project Approval</title>"
             << "<style>body{font-family:Arial,sans-serif;margin:24px;}table{border-collapse:collapse;width:100%;}"
             << "th,td{border:1px solid #ddd;padding:8px;text-align:left;}th{background:#f5f5f5;}"
             << ".approved{color:#0a7d18;font-weight:bold;}.pending{color:#946200;font-weight:bold;}.rejected{color:#b00020;font-weight:bold;}</style>"
             << "</head><body>";
        html << "<h1>" << EscapeHtml(document.Title) << "</h1>";
        html << "<p><strong>Project:</strong> " << EscapeHtml(project.ProjectName) << "<br><strong>Fingerprint:</strong> "
             << EscapeHtml(verification.ProjectFingerprint) << "<br><strong>Decision:</strong> <span class=\"" << ToString(verification.OverallDecision)
             << "\">" << ToString(verification.OverallDecision) << "</span></p>";
        html << "<table><thead><tr><th>Role</th><th>Satisfied</th><th>Latest signer</th><th>Timestamp</th><th>Notes</th></tr></thead><tbody>";
        for (const auto& role : verification.RoleStatuses)
        {
            html << "<tr><td>" << EscapeHtml(role.Role) << "</td><td>" << (role.IsSatisfied ? "YES" : "NO")
                 << "</td><td>" << EscapeHtml(role.LatestSigner) << "</td><td>" << EscapeHtml(role.LatestTimestampUtc)
                 << "</td><td>" << EscapeHtml(role.LatestNotes) << "</td></tr>";
        }
        html << "</tbody></table><h2>Notes</h2><ul>";
        for (const auto& note : verification.Notes)
        {
            html << "<li>" << EscapeHtml(note) << "</li>";
        }
        html << "</ul></body></html>";
        return html.str();
    }

    void ProjectApprovalWorkflow::SaveHtmlReport(
        const apex::io::StudioProject& project,
        const ProjectApprovalDocument& document,
        const ProjectApprovalVerification& verification,
        const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write approval report: " + filePath.string());
        }
        stream << BuildHtmlReport(project, document, verification);
    }
}
