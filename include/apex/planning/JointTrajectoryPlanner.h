#pragma once

#include "apex/core/RobotTypes.h"
#include "apex/core/SerialRobotModel.h"
#include "apex/workcell/CollisionWorld.h"

#include <vector>

namespace apex::planning
{
    struct TrajectorySample
    {
        double TimeSeconds = 0.0;
        apex::core::JointState State;
        apex::core::TcpPose Tcp;
        bool HasCollision = false;
    };

    struct TrajectoryPlan
    {
        std::vector<TrajectorySample> Samples;
        bool IsCompletelyCollisionFree = true;
    };

    class JointTrajectoryPlanner
    {
    public:
        [[nodiscard]] TrajectoryPlan PlanLinearJointMotion(
            const apex::core::SerialRobotModel& robot,
            const apex::core::JointState& start,
            const apex::core::JointState& goal,
            std::size_t sampleCount,
            double durationSeconds,
            const apex::workcell::CollisionWorld& world,
            double linkSafetyRadius) const;
    };
}
