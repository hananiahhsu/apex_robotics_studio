#pragma once

#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/workcell/CollisionWorld.h"

#include <string>
#include <vector>

namespace apex::analysis
{
    struct JointVelocityMetric
    {
        std::string JointName;
        double MaxAbsVelocityRadPerSec = 0.0;
        double PreferredVelocityLimitRadPerSec = 0.0;
        bool ExceedsPreferredVelocityLimit = false;
    };

    struct TrajectoryQualityReport
    {
        bool CollisionFree = true;
        std::size_t CollisionSampleCount = 0;
        double PathLengthMeters = 0.0;
        double MaxTcpStepMeters = 0.0;
        double EstimatedCycleTimeSeconds = 0.0;
        double AverageTcpSpeedMetersPerSecond = 0.0;
        double PeakTcpSpeedMetersPerSecond = 0.0;
        std::size_t FirstCollisionSampleIndex = static_cast<std::size_t>(-1);
        std::vector<JointVelocityMetric> JointVelocityMetrics;
        std::vector<std::string> Warnings;
    };

    class TrajectoryAnalyzer
    {
    public:
        [[nodiscard]] TrajectoryQualityReport Analyze(
            const apex::core::SerialRobotModel& robot,
            const apex::workcell::CollisionWorld& world,
            const apex::planning::TrajectoryPlan& plan,
            double linkSafetyRadius,
            double preferredJointVelocityLimitRadPerSec = 1.5) const;
    };
}
