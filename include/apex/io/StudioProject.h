#pragma once

#include "apex/core/RobotTypes.h"
#include "apex/workcell/CollisionWorld.h"
#include "apex/job/RobotJob.h"
#include "apex/core/MathTypes.h"

#include <string>
#include <vector>

namespace apex::io
{
    struct MotionRequest
    {
        apex::core::JointState Start;
        apex::core::JointState Goal;
        std::size_t SampleCount = 16;
        double DurationSeconds = 4.0;
        double LinkSafetyRadius = 0.08;
    };

    struct ProjectMetadata
    {
        std::string CellName;
        std::string ProcessFamily;
        std::string Owner;
        std::string Notes;
    };


    struct RobotDescriptionSource
    {
        std::string SourcePath;
        std::string ExpandedDescriptionPath;
        std::string SourceKind = "manual";
        std::vector<std::string> PackageDependencies;
    };

    struct MeshResource
    {
        std::string LinkName;
        std::string Role;
        std::string Uri;
        std::string ResolvedPath;
        std::string PackageName;
        apex::core::Vec3 Scale = {1.0, 1.0, 1.0};
    };

    struct QualityGateCriteria
    {
        bool RequireCollisionFree = true;
        std::size_t MaxErrorFindings = 0;
        std::size_t MaxWarningFindings = 3;
        double MaxPeakTcpSpeedMetersPerSecond = 1.8;
        double MaxAverageTcpSpeedMetersPerSecond = 1.2;
        double PreferredJointVelocityLimitRadPerSec = 1.5;
    };

    struct StudioProject
    {
        int SchemaVersion = 1;
        std::string ProjectName;
        std::string RobotName;
        std::vector<apex::core::JointDefinition> Joints;
        std::vector<apex::workcell::Obstacle> Obstacles;
        MotionRequest Motion;
        apex::job::RobotJob Job;
        ProjectMetadata Metadata;
        QualityGateCriteria QualityGate;
        RobotDescriptionSource DescriptionSource;
        std::vector<MeshResource> MeshResources;
    };
}
