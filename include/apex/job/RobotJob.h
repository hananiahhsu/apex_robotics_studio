#pragma once

#include "apex/core/RobotTypes.h"

#include <string>
#include <vector>

namespace apex::job
{
    struct NamedWaypoint
    {
        std::string Name;
        apex::core::JointState State;
    };

    struct JobSegment
    {
        std::string Name;
        std::string StartWaypointName;
        std::string GoalWaypointName;
        std::size_t SampleCount = 12;
        double DurationSeconds = 2.0;
        std::string ProcessTag;
    };

    struct RobotJob
    {
        std::string Name;
        std::vector<NamedWaypoint> Waypoints;
        std::vector<JobSegment> Segments;
        double LinkSafetyRadius = 0.08;

        [[nodiscard]] bool Empty() const noexcept
        {
            return Waypoints.empty() && Segments.empty();
        }
    };
}
