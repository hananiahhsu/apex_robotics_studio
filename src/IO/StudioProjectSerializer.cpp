#include "apex/io/StudioProjectSerializer.h"

#include "apex/core/MathTypes.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace apex::io
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
            const auto isSpace = [](unsigned char character) { return std::isspace(character) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        std::vector<std::string> Split(const std::string& text, char delimiter)
        {
            std::vector<std::string> tokens;
            std::stringstream stream(text);
            std::string token;
            while (std::getline(stream, token, delimiter))
            {
                tokens.push_back(Trim(token));
            }
            return tokens;
        }

        double ParseDouble(const std::string& text, const std::string& fieldName)
        {
            const std::string trimmed = Trim(text);
            char* end = nullptr;
            const double value = std::strtod(trimmed.c_str(), &end);
            if (end == trimmed.c_str() || *end != '\0')
            {
                throw std::invalid_argument("Invalid floating-point value for field '" + fieldName + "': '" + text + "'.");
            }
            return value;
        }

        int ParseInt(const std::string& text, const std::string& fieldName)
        {
            const std::string trimmed = Trim(text);
            int value = 0;
            const auto [pointer, errorCode] = std::from_chars(trimmed.data(), trimmed.data() + trimmed.size(), value);
            if (errorCode != std::errc{} || pointer != trimmed.data() + trimmed.size())
            {
                throw std::invalid_argument("Invalid integer value for field '" + fieldName + "': '" + text + "'.");
            }
            return value;
        }

        bool ParseBool(const std::string& text, const std::string& fieldName)
        {
            const std::string trimmed = Trim(text);
            if (trimmed == "true" || trimmed == "1" || trimmed == "yes" || trimmed == "on")
            {
                return true;
            }
            if (trimmed == "false" || trimmed == "0" || trimmed == "no" || trimmed == "off")
            {
                return false;
            }
            throw std::invalid_argument("Invalid boolean value for field '" + fieldName + "': '" + text + "'.");
        }

        std::size_t ParseSize(const std::string& text, const std::string& fieldName)
        {
            const int signedValue = ParseInt(text, fieldName);
            if (signedValue < 0)
            {
                throw std::invalid_argument("Field '" + fieldName + "' must be non-negative.");
            }
            return static_cast<std::size_t>(signedValue);
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

        apex::core::Vec3 ParseVec3(const std::string& text, const std::string& fieldName)
        {
            const std::vector<std::string> parts = Split(text, ',');
            if (parts.size() != 3)
            {
                throw std::invalid_argument("Field '" + fieldName + "' must contain exactly three comma-separated values.");
            }
            return {
                ParseDouble(parts[0], fieldName + ".x"),
                ParseDouble(parts[1], fieldName + ".y"),
                ParseDouble(parts[2], fieldName + ".z") };
        }

        std::vector<double> ParseAngleListDegrees(const std::string& text, const std::string& fieldName)
        {
            const std::vector<std::string> parts = Split(text, ',');
            std::vector<double> angles;
            angles.reserve(parts.size());
            for (std::size_t index = 0; index < parts.size(); ++index)
            {
                angles.push_back(apex::core::DegreesToRadians(ParseDouble(parts[index], fieldName + "[" + std::to_string(index) + "]")));
            }
            return angles;
        }

        std::vector<ParsedSection> ParseSections(std::istream& stream)
        {
            std::vector<ParsedSection> sections;
            ParsedSection* currentSection = nullptr;
            std::string line;
            std::size_t lineNumber = 0;
            while (std::getline(stream, line))
            {
                ++lineNumber;
                const std::string trimmed = Trim(line);
                if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';')
                {
                    continue;
                }
                if (trimmed.front() == '[' && trimmed.back() == ']')
                {
                    sections.push_back({ trimmed.substr(1, trimmed.size() - 2), {} });
                    currentSection = &sections.back();
                    continue;
                }
                if (currentSection == nullptr)
                {
                    throw std::invalid_argument("Key-value content found before any section at line " + std::to_string(lineNumber) + ".");
                }
                const std::size_t separator = trimmed.find('=');
                if (separator == std::string::npos)
                {
                    throw std::invalid_argument("Expected key=value format at line " + std::to_string(lineNumber) + ".");
                }
                const std::string key = Trim(trimmed.substr(0, separator));
                const std::string value = Trim(trimmed.substr(separator + 1));
                if (key.empty())
                {
                    throw std::invalid_argument("Empty key at line " + std::to_string(lineNumber) + ".");
                }
                currentSection->Values[key] = value;
            }
            return sections;
        }

        std::string JoinDegrees(const std::vector<double>& values)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(6);
            for (std::size_t index = 0; index < values.size(); ++index)
            {
                if (index > 0)
                {
                    builder << ',';
                }
                builder << apex::core::RadiansToDegrees(values[index]);
            }
            return builder.str();
        }

        std::string FormatVec3(const apex::core::Vec3& value)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(6);
            builder << value.X << ',' << value.Y << ',' << value.Z;
            return builder.str();
        }

        bool HasMetadata(const ProjectMetadata& metadata)
        {
            return !metadata.CellName.empty() || !metadata.ProcessFamily.empty() || !metadata.Owner.empty() || !metadata.Notes.empty();
        }

        bool HasDescriptionSource(const RobotDescriptionSource& source)
        {
            return !source.SourcePath.empty() || !source.ExpandedDescriptionPath.empty() || source.SourceKind != "manual" || !source.PackageDependencies.empty();
        }

        bool HasMeshResources(const StudioProject& project)
        {
            return !project.MeshResources.empty();
        }

        bool HasCustomQualityGate(const QualityGateCriteria& gate)
        {
            const QualityGateCriteria defaults;
            return gate.RequireCollisionFree != defaults.RequireCollisionFree ||
                   gate.MaxErrorFindings != defaults.MaxErrorFindings ||
                   gate.MaxWarningFindings != defaults.MaxWarningFindings ||
                   gate.MaxPeakTcpSpeedMetersPerSecond != defaults.MaxPeakTcpSpeedMetersPerSecond ||
                   gate.MaxAverageTcpSpeedMetersPerSecond != defaults.MaxAverageTcpSpeedMetersPerSecond ||
                   gate.PreferredJointVelocityLimitRadPerSec != defaults.PreferredJointVelocityLimitRadPerSec;
        }

        StudioProject ParseProjectFromSections(const std::vector<ParsedSection>& sections)
        {
            StudioProject project;
            bool hasProjectSection = false;
            bool hasMotionSection = false;

            for (const ParsedSection& section : sections)
            {
                if (section.Name == "project")
                {
                    project.SchemaVersion = ParseInt(GetRequired(section.Values, "schema_version", section.Name), "schema_version");
                    project.ProjectName = GetRequired(section.Values, "project_name", section.Name);
                    project.RobotName = GetRequired(section.Values, "robot_name", section.Name);
                    hasProjectSection = true;
                }
                else if (section.Name == "metadata")
                {
                    const auto itCell = section.Values.find("cell_name");
                    const auto itProcess = section.Values.find("process_family");
                    const auto itOwner = section.Values.find("owner");
                    const auto itNotes = section.Values.find("notes");
                    if (itCell != section.Values.end()) { project.Metadata.CellName = itCell->second; }
                    if (itProcess != section.Values.end()) { project.Metadata.ProcessFamily = itProcess->second; }
                    if (itOwner != section.Values.end()) { project.Metadata.Owner = itOwner->second; }
                    if (itNotes != section.Values.end()) { project.Metadata.Notes = itNotes->second; }
                }
                else if (section.Name == "robot_description")
                {
                    const auto itSource = section.Values.find("source_path");
                    const auto itExpanded = section.Values.find("expanded_description_path");
                    const auto itKind = section.Values.find("source_kind");
                    const auto itDeps = section.Values.find("package_dependencies");
                    if (itSource != section.Values.end()) { project.DescriptionSource.SourcePath = itSource->second; }
                    if (itExpanded != section.Values.end()) { project.DescriptionSource.ExpandedDescriptionPath = itExpanded->second; }
                    if (itKind != section.Values.end()) { project.DescriptionSource.SourceKind = itKind->second; }
                    if (itDeps != section.Values.end())
                    {
                        std::stringstream depStream(itDeps->second);
                        std::string token;
                        while (std::getline(depStream, token, ';'))
                        {
                            token = Trim(token);
                            if (!token.empty())
                            {
                                project.DescriptionSource.PackageDependencies.push_back(token);
                            }
                        }
                    }
                }
                else if (section.Name == "mesh")
                {
                    apex::io::MeshResource mesh;
                    mesh.LinkName = GetRequired(section.Values, "link_name", section.Name);
                    mesh.Role = GetRequired(section.Values, "role", section.Name);
                    mesh.Uri = GetRequired(section.Values, "uri", section.Name);
                    const auto itResolved = section.Values.find("resolved_path");
                    if (itResolved != section.Values.end())
                    {
                        mesh.ResolvedPath = itResolved->second;
                    }
                    const auto itPackage = section.Values.find("package_name");
                    if (itPackage != section.Values.end())
                    {
                        mesh.PackageName = itPackage->second;
                    }
                    const auto itScale = section.Values.find("scale");
                    if (itScale != section.Values.end())
                    {
                        mesh.Scale = ParseVec3(itScale->second, "scale");
                    }
                    project.MeshResources.push_back(mesh);
                }
                else if (section.Name == "quality_gate")
                {
                    if (section.Values.find("require_collision_free") != section.Values.end())
                    {
                        project.QualityGate.RequireCollisionFree = ParseBool(GetRequired(section.Values, "require_collision_free", section.Name), "require_collision_free");
                    }
                    if (section.Values.find("max_error_findings") != section.Values.end())
                    {
                        project.QualityGate.MaxErrorFindings = ParseSize(GetRequired(section.Values, "max_error_findings", section.Name), "max_error_findings");
                    }
                    if (section.Values.find("max_warning_findings") != section.Values.end())
                    {
                        project.QualityGate.MaxWarningFindings = ParseSize(GetRequired(section.Values, "max_warning_findings", section.Name), "max_warning_findings");
                    }
                    if (section.Values.find("max_peak_tcp_speed_mps") != section.Values.end())
                    {
                        project.QualityGate.MaxPeakTcpSpeedMetersPerSecond = ParseDouble(GetRequired(section.Values, "max_peak_tcp_speed_mps", section.Name), "max_peak_tcp_speed_mps");
                    }
                    if (section.Values.find("max_average_tcp_speed_mps") != section.Values.end())
                    {
                        project.QualityGate.MaxAverageTcpSpeedMetersPerSecond = ParseDouble(GetRequired(section.Values, "max_average_tcp_speed_mps", section.Name), "max_average_tcp_speed_mps");
                    }
                    if (section.Values.find("preferred_joint_velocity_limit_rad_per_sec") != section.Values.end())
                    {
                        project.QualityGate.PreferredJointVelocityLimitRadPerSec = ParseDouble(GetRequired(section.Values, "preferred_joint_velocity_limit_rad_per_sec", section.Name), "preferred_joint_velocity_limit_rad_per_sec");
                    }
                }
                else if (section.Name == "joint")
                {
                    apex::core::JointDefinition joint;
                    joint.Name = GetRequired(section.Values, "name", section.Name);
                    joint.LinkLength = ParseDouble(GetRequired(section.Values, "link_length", section.Name), "link_length");
                    joint.MinAngleRad = apex::core::DegreesToRadians(ParseDouble(GetRequired(section.Values, "min_angle_deg", section.Name), "min_angle_deg"));
                    joint.MaxAngleRad = apex::core::DegreesToRadians(ParseDouble(GetRequired(section.Values, "max_angle_deg", section.Name), "max_angle_deg"));
                    project.Joints.push_back(joint);
                }
                else if (section.Name == "obstacle")
                {
                    apex::workcell::Obstacle obstacle;
                    obstacle.Name = GetRequired(section.Values, "name", section.Name);
                    obstacle.Bounds.Min = ParseVec3(GetRequired(section.Values, "min", section.Name), "min");
                    obstacle.Bounds.Max = ParseVec3(GetRequired(section.Values, "max", section.Name), "max");
                    project.Obstacles.push_back(obstacle);
                }
                else if (section.Name == "motion")
                {
                    project.Motion.Start.JointAnglesRad = ParseAngleListDegrees(GetRequired(section.Values, "start_angles_deg", section.Name), "start_angles_deg");
                    project.Motion.Goal.JointAnglesRad = ParseAngleListDegrees(GetRequired(section.Values, "goal_angles_deg", section.Name), "goal_angles_deg");
                    project.Motion.SampleCount = ParseSize(GetRequired(section.Values, "sample_count", section.Name), "sample_count");
                    project.Motion.DurationSeconds = ParseDouble(GetRequired(section.Values, "duration_seconds", section.Name), "duration_seconds");
                    project.Motion.LinkSafetyRadius = ParseDouble(GetRequired(section.Values, "link_safety_radius", section.Name), "link_safety_radius");
                    hasMotionSection = true;
                }
                else if (section.Name == "job")
                {
                    project.Job.Name = GetRequired(section.Values, "name", section.Name);
                    project.Job.LinkSafetyRadius = ParseDouble(GetRequired(section.Values, "link_safety_radius", section.Name), "link_safety_radius");
                }
                else if (section.Name == "waypoint")
                {
                    apex::job::NamedWaypoint waypoint;
                    waypoint.Name = GetRequired(section.Values, "name", section.Name);
                    waypoint.State.JointAnglesRad = ParseAngleListDegrees(GetRequired(section.Values, "angles_deg", section.Name), "angles_deg");
                    project.Job.Waypoints.push_back(waypoint);
                }
                else if (section.Name == "segment")
                {
                    apex::job::JobSegment segment;
                    segment.Name = GetRequired(section.Values, "name", section.Name);
                    segment.StartWaypointName = GetRequired(section.Values, "start_waypoint", section.Name);
                    segment.GoalWaypointName = GetRequired(section.Values, "goal_waypoint", section.Name);
                    segment.SampleCount = ParseSize(GetRequired(section.Values, "sample_count", section.Name), "sample_count");
                    segment.DurationSeconds = ParseDouble(GetRequired(section.Values, "duration_seconds", section.Name), "duration_seconds");
                    const auto it = section.Values.find("process_tag");
                    if (it != section.Values.end())
                    {
                        segment.ProcessTag = it->second;
                    }
                    project.Job.Segments.push_back(segment);
                }
            }

            if (!hasProjectSection)
            {
                throw std::invalid_argument("Project file is missing [project] section.");
            }
            if (!hasMotionSection)
            {
                throw std::invalid_argument("Project file is missing [motion] section.");
            }
            if (project.SchemaVersion != 1 && project.SchemaVersion != 2 && project.SchemaVersion != 3 && project.SchemaVersion != 4 && project.SchemaVersion != 5)
            {
                throw std::invalid_argument("Unsupported schema version: " + std::to_string(project.SchemaVersion) + ".");
            }
            if (project.Joints.empty())
            {
                throw std::invalid_argument("Project must define at least one [joint] section.");
            }
            if (project.Motion.Start.JointAnglesRad.size() != project.Joints.size() ||
                project.Motion.Goal.JointAnglesRad.size() != project.Joints.size())
            {
                throw std::invalid_argument("Motion start/goal state dimensions must match the number of joints.");
            }
            for (const auto& waypoint : project.Job.Waypoints)
            {
                if (waypoint.State.JointAnglesRad.size() != project.Joints.size())
                {
                    throw std::invalid_argument("Waypoint '" + waypoint.Name + "' state dimension must match the number of joints.");
                }
            }
            if (!project.Job.Empty() && project.Job.Name.empty())
            {
                project.Job.Name = "Imported Job";
            }
            return project;
        }

        std::string SerializeProjectToString(const StudioProject& project)
        {
            std::ostringstream stream;
            const bool hasJob = !project.Job.Empty();
            const bool hasMetadata = HasMetadata(project.Metadata);
            const bool hasQualityGate = HasCustomQualityGate(project.QualityGate);
            const bool hasDescription = HasDescriptionSource(project.DescriptionSource);
            const bool hasMeshes = HasMeshResources(project);
            int schemaVersion = (hasMetadata || hasQualityGate) ? 3 : (hasJob ? 2 : 1);
            if (hasDescription || hasMeshes)
            {
                schemaVersion = 4;
            }
            for (const auto& mesh : project.MeshResources)
            {
                if (!mesh.ResolvedPath.empty() || !mesh.PackageName.empty())
                {
                    schemaVersion = 5;
                    break;
                }
            }

            stream << "; ApexRoboticsStudio project file\n"
                   << "; Schema versioned for interview and product-demo use\n\n"
                   << "[project]\n"
                   << "schema_version=" << schemaVersion << '\n'
                   << "project_name=" << project.ProjectName << '\n'
                   << "robot_name=" << project.RobotName << "\n\n";

            if (hasMetadata)
            {
                stream << "[metadata]\n"
                       << "cell_name=" << project.Metadata.CellName << '\n'
                       << "process_family=" << project.Metadata.ProcessFamily << '\n'
                       << "owner=" << project.Metadata.Owner << '\n'
                       << "notes=" << project.Metadata.Notes << "\n\n";
            }

            if (hasDescription)
            {
                stream << "[robot_description]\n"
                       << "source_path=" << project.DescriptionSource.SourcePath << '\n'
                       << "expanded_description_path=" << project.DescriptionSource.ExpandedDescriptionPath << '\n'
                       << "source_kind=" << project.DescriptionSource.SourceKind << '\n';
                if (!project.DescriptionSource.PackageDependencies.empty())
                {
                    stream << "package_dependencies=";
                    for (std::size_t index = 0; index < project.DescriptionSource.PackageDependencies.size(); ++index)
                    {
                        if (index > 0)
                        {
                            stream << ';';
                        }
                        stream << project.DescriptionSource.PackageDependencies[index];
                    }
                    stream << '\n';
                }
                stream << "\n";
            }

            if (hasQualityGate)
            {
                stream << "[quality_gate]\n"
                       << "require_collision_free=" << (project.QualityGate.RequireCollisionFree ? "true" : "false") << '\n'
                       << "max_error_findings=" << project.QualityGate.MaxErrorFindings << '\n'
                       << "max_warning_findings=" << project.QualityGate.MaxWarningFindings << '\n'
                       << "max_peak_tcp_speed_mps=" << project.QualityGate.MaxPeakTcpSpeedMetersPerSecond << '\n'
                       << "max_average_tcp_speed_mps=" << project.QualityGate.MaxAverageTcpSpeedMetersPerSecond << '\n'
                       << "preferred_joint_velocity_limit_rad_per_sec=" << project.QualityGate.PreferredJointVelocityLimitRadPerSec << "\n\n";
            }

            for (const auto& mesh : project.MeshResources)
            {
                stream << "[mesh]\n"
                       << "link_name=" << mesh.LinkName << '\n'
                       << "role=" << mesh.Role << '\n'
                       << "uri=" << mesh.Uri << '\n';
                if (!mesh.ResolvedPath.empty())
                {
                    stream << "resolved_path=" << mesh.ResolvedPath << '\n';
                }
                if (!mesh.PackageName.empty())
                {
                    stream << "package_name=" << mesh.PackageName << '\n';
                }
                stream << "scale=" << FormatVec3(mesh.Scale) << "\n\n";
            }

            for (const auto& joint : project.Joints)
            {
                stream << "[joint]\n"
                       << "name=" << joint.Name << '\n'
                       << "link_length=" << joint.LinkLength << '\n'
                       << "min_angle_deg=" << apex::core::RadiansToDegrees(joint.MinAngleRad) << '\n'
                       << "max_angle_deg=" << apex::core::RadiansToDegrees(joint.MaxAngleRad) << "\n\n";
            }

            for (const auto& obstacle : project.Obstacles)
            {
                stream << "[obstacle]\n"
                       << "name=" << obstacle.Name << '\n'
                       << "min=" << FormatVec3(obstacle.Bounds.Min) << '\n'
                       << "max=" << FormatVec3(obstacle.Bounds.Max) << "\n\n";
            }

            stream << "[motion]\n"
                   << "start_angles_deg=" << JoinDegrees(project.Motion.Start.JointAnglesRad) << '\n'
                   << "goal_angles_deg=" << JoinDegrees(project.Motion.Goal.JointAnglesRad) << '\n'
                   << "sample_count=" << project.Motion.SampleCount << '\n'
                   << "duration_seconds=" << project.Motion.DurationSeconds << '\n'
                   << "link_safety_radius=" << project.Motion.LinkSafetyRadius << "\n\n";

            if (hasJob)
            {
                stream << "[job]\n"
                       << "name=" << project.Job.Name << '\n'
                       << "link_safety_radius=" << project.Job.LinkSafetyRadius << "\n\n";

                for (const auto& waypoint : project.Job.Waypoints)
                {
                    stream << "[waypoint]\n"
                           << "name=" << waypoint.Name << '\n'
                           << "angles_deg=" << JoinDegrees(waypoint.State.JointAnglesRad) << "\n\n";
                }

                for (const auto& segment : project.Job.Segments)
                {
                    stream << "[segment]\n"
                           << "name=" << segment.Name << '\n'
                           << "start_waypoint=" << segment.StartWaypointName << '\n'
                           << "goal_waypoint=" << segment.GoalWaypointName << '\n'
                           << "sample_count=" << segment.SampleCount << '\n'
                           << "duration_seconds=" << segment.DurationSeconds << '\n'
                           << "process_tag=" << segment.ProcessTag << "\n\n";
                }
            }
            return stream.str();
        }
    }

    StudioProject StudioProjectSerializer::LoadFromFile(const std::filesystem::path& filePath) const
    {
        std::ifstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to open project file: " + filePath.string());
        }
        return ParseProjectFromSections(ParseSections(stream));
    }

    StudioProject StudioProjectSerializer::LoadFromString(const std::string& text) const
    {
        std::istringstream stream(text);
        return ParseProjectFromSections(ParseSections(stream));
    }

    void StudioProjectSerializer::SaveToFile(const StudioProject& project, const std::filesystem::path& filePath) const
    {
        std::ofstream stream(filePath);
        if (!stream)
        {
            throw std::runtime_error("Failed to write project file: " + filePath.string());
        }
        stream << SerializeProjectToString(project);
    }

    std::string StudioProjectSerializer::SaveToString(const StudioProject& project) const
    {
        return SerializeProjectToString(project);
    }
}
