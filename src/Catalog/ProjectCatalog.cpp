#include "apex/catalog/ProjectCatalog.h"

#include "apex/core/MathTypes.h"

#include <sstream>

namespace apex::catalog
{
    namespace
    {
        std::string FormatAnglesDegrees(const std::vector<double>& values)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(1);
            for (std::size_t index = 0; index < values.size(); ++index)
            {
                if (index > 0)
                {
                    builder << ", ";
                }
                builder << apex::core::RadiansToDegrees(values[index]);
            }
            return builder.str();
        }
    }

    ProjectCatalog ProjectCatalogBuilder::Build(const apex::io::StudioProject& project) const
    {
        ProjectCatalog catalog;
        catalog.ProjectName = project.ProjectName;
        catalog.RobotName = project.RobotName;
        catalog.JointCount = project.Joints.size();
        catalog.ObstacleCount = project.Obstacles.size();
        catalog.WaypointCount = project.Job.Waypoints.size();
        catalog.SegmentCount = project.Job.Segments.size();
        catalog.MeshCount = project.MeshResources.size();

        catalog.Items.push_back({0, "project", project.ProjectName, "schema=" + std::to_string(project.SchemaVersion)});
        catalog.Items.push_back({1, "robot", project.RobotName, std::to_string(project.Joints.size()) + " joints"});
        if (!project.DescriptionSource.SourcePath.empty() || !project.DescriptionSource.ExpandedDescriptionPath.empty())
        {
            catalog.Items.push_back({2, "description", project.DescriptionSource.SourceKind, project.DescriptionSource.SourcePath});
            if (!project.DescriptionSource.PackageDependencies.empty())
            {
                catalog.Items.push_back({2, "packages", "Description Packages", std::to_string(project.DescriptionSource.PackageDependencies.size()) + " entries"});
                for (const auto& dependency : project.DescriptionSource.PackageDependencies)
                {
                    catalog.Items.push_back({3, "package", dependency, "robot description dependency"});
                }
            }
        }
        for (const auto& joint : project.Joints)
        {
            std::ostringstream detail;
            detail.setf(std::ios::fixed);
            detail.precision(3);
            detail << "link=" << joint.LinkLength << "m range=["
                   << apex::core::RadiansToDegrees(joint.MinAngleRad) << ", "
                   << apex::core::RadiansToDegrees(joint.MaxAngleRad) << "]deg";
            catalog.Items.push_back({2, "joint", joint.Name, detail.str()});
        }

        if (!project.MeshResources.empty())
        {
            catalog.Items.push_back({1, "meshes", "Robot Mesh Resources", std::to_string(project.MeshResources.size()) + " entries"});
            for (const auto& mesh : project.MeshResources)
            {
                std::string details = mesh.Uri + " scale=" + apex::core::ToString(mesh.Scale);
                if (!mesh.PackageName.empty())
                {
                    details += " package=" + mesh.PackageName;
                }
                if (!mesh.ResolvedPath.empty())
                {
                    details += " resolved=" + mesh.ResolvedPath;
                }
                catalog.Items.push_back({2, mesh.Role, mesh.LinkName, details});
            }
        }

        catalog.Items.push_back({1, "motion", "Primary Motion", std::to_string(project.Motion.SampleCount) + " samples"});
        catalog.Items.push_back({2, "state", "Start", FormatAnglesDegrees(project.Motion.Start.JointAnglesRad)});
        catalog.Items.push_back({2, "state", "Goal", FormatAnglesDegrees(project.Motion.Goal.JointAnglesRad)});

        catalog.Items.push_back({1, "workcell", "Obstacles", std::to_string(project.Obstacles.size()) + " entries"});
        for (const auto& obstacle : project.Obstacles)
        {
            catalog.Items.push_back({2, "obstacle", obstacle.Name,
                "min=" + apex::core::ToString(obstacle.Bounds.Min) + " max=" + apex::core::ToString(obstacle.Bounds.Max)});
        }

        if (!project.Job.Empty())
        {
            catalog.Items.push_back({1, "job", project.Job.Name, std::to_string(project.Job.Segments.size()) + " segments"});
            for (const auto& waypoint : project.Job.Waypoints)
            {
                catalog.Items.push_back({2, "waypoint", waypoint.Name, FormatAnglesDegrees(waypoint.State.JointAnglesRad)});
            }
            for (const auto& segment : project.Job.Segments)
            {
                std::ostringstream detail;
                detail << segment.StartWaypointName << " -> " << segment.GoalWaypointName
                       << " | samples=" << segment.SampleCount
                       << " | duration=" << segment.DurationSeconds << "s";
                if (!segment.ProcessTag.empty())
                {
                    detail << " | tag=" << segment.ProcessTag;
                }
                catalog.Items.push_back({2, "segment", segment.Name, detail.str()});
            }
        }

        return catalog;
    }

    std::string ProjectCatalogBuilder::BuildTextTree(const ProjectCatalog& catalog) const
    {
        std::ostringstream stream;
        stream << "Project Catalog\n";
        for (const auto& item : catalog.Items)
        {
            stream << std::string(static_cast<std::size_t>(item.Depth) * 2, ' ')
                   << "- [" << item.Kind << "] " << item.Name;
            if (!item.Details.empty())
            {
                stream << " :: " << item.Details;
            }
            stream << '\n';
        }
        return stream.str();
    }
}
