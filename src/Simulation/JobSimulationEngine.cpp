#include "apex/simulation/JobSimulationEngine.h"

#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace apex::simulation
{
    JobSimulationResult JobSimulationEngine::Simulate(
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world,
        const apex::job::RobotJob& job,
        double preferredJointVelocityLimitRadPerSec) const
    {
        if (job.Waypoints.empty())
        {
            throw std::invalid_argument("Robot job must define at least one waypoint.");
        }
        if (job.Segments.empty())
        {
            throw std::invalid_argument("Robot job must define at least one segment.");
        }

        std::unordered_map<std::string, const apex::job::NamedWaypoint*> waypointMap;
        for (const auto& waypoint : job.Waypoints)
        {
            if (!robot.IsStateDimensionValid(waypoint.State))
            {
                throw std::invalid_argument("Waypoint '" + waypoint.Name + "' has an invalid state dimension.");
            }
            waypointMap[waypoint.Name] = &waypoint;
        }

        const apex::planning::JointTrajectoryPlanner planner;
        const apex::analysis::TrajectoryAnalyzer analyzer;
        JobSimulationResult result;
        result.JobName = job.Name;

        for (const auto& segment : job.Segments)
        {
            const auto startIt = waypointMap.find(segment.StartWaypointName);
            const auto goalIt = waypointMap.find(segment.GoalWaypointName);
            if (startIt == waypointMap.end())
            {
                throw std::invalid_argument("Job segment '" + segment.Name + "' references missing start waypoint '" + segment.StartWaypointName + "'.");
            }
            if (goalIt == waypointMap.end())
            {
                throw std::invalid_argument("Job segment '" + segment.Name + "' references missing goal waypoint '" + segment.GoalWaypointName + "'.");
            }

            JobSegmentResult segmentResult;
            segmentResult.Segment = segment;
            segmentResult.Plan = planner.PlanLinearJointMotion(
                robot,
                startIt->second->State,
                goalIt->second->State,
                segment.SampleCount,
                segment.DurationSeconds,
                world,
                job.LinkSafetyRadius);
            segmentResult.Quality = analyzer.Analyze(robot, world, segmentResult.Plan, job.LinkSafetyRadius, preferredJointVelocityLimitRadPerSec);

            if (!segmentResult.Quality.CollisionFree)
            {
                result.CollisionFree = false;
                result.Warnings.push_back("Segment '" + segment.Name + "' is not collision-free.");
            }
            if (segmentResult.Quality.PeakTcpSpeedMetersPerSecond > 1.2)
            {
                std::ostringstream builder;
                builder << "Segment '" << segment.Name << "' peak TCP speed is "
                        << segmentResult.Quality.PeakTcpSpeedMetersPerSecond << " m/s and should be reviewed.";
                result.Warnings.push_back(builder.str());
            }

            result.TotalSamples += segmentResult.Plan.Samples.size();
            result.TotalDurationSeconds += segmentResult.Quality.EstimatedCycleTimeSeconds;
            result.TotalPathLengthMeters += segmentResult.Quality.PathLengthMeters;
            result.SegmentResults.push_back(std::move(segmentResult));
        }

        return result;
    }
}
