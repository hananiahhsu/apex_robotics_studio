#pragma once

#include "apex/analysis/TrajectoryAnalyzer.h"
#include "apex/job/RobotJob.h"
#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/workcell/CollisionWorld.h"

#include <string>
#include <vector>

namespace apex::simulation
{
    struct JobSegmentResult
    {
        apex::job::JobSegment Segment;
        apex::planning::TrajectoryPlan Plan;
        apex::analysis::TrajectoryQualityReport Quality;
    };

    struct JobSimulationResult
    {
        std::string JobName;
        std::vector<JobSegmentResult> SegmentResults;
        bool CollisionFree = true;
        std::size_t TotalSamples = 0;
        double TotalDurationSeconds = 0.0;
        double TotalPathLengthMeters = 0.0;
        std::vector<std::string> Warnings;
    };

    class JobSimulationEngine
    {
    public:
        [[nodiscard]] JobSimulationResult Simulate(
            const apex::core::SerialRobotModel& robot,
            const apex::workcell::CollisionWorld& world,
            const apex::job::RobotJob& job,
            double preferredJointVelocityLimitRadPerSec = 1.5) const;
    };
}
