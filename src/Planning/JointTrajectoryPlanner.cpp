#include "apex/planning/JointTrajectoryPlanner.h"

#include <stdexcept>

namespace apex::planning
{
    namespace
    {
        apex::core::JointState Interpolate(
            const apex::core::JointState& start,
            const apex::core::JointState& goal,
            double t)
        {
            apex::core::JointState state;
            state.JointAnglesRad.resize(start.JointAnglesRad.size());
            for (std::size_t index = 0; index < start.JointAnglesRad.size(); ++index)
            {
                state.JointAnglesRad[index] = start.JointAnglesRad[index] +
                    (goal.JointAnglesRad[index] - start.JointAnglesRad[index]) * t;
            }
            return state;
        }
    }

    TrajectoryPlan JointTrajectoryPlanner::PlanLinearJointMotion(
        const apex::core::SerialRobotModel& robot,
        const apex::core::JointState& start,
        const apex::core::JointState& goal,
        std::size_t sampleCount,
        double durationSeconds,
        const apex::workcell::CollisionWorld& world,
        double linkSafetyRadius) const
    {
        if (!robot.IsStateDimensionValid(start) || !robot.IsStateDimensionValid(goal))
        {
            throw std::invalid_argument("Planner input state dimensions do not match the robot model.");
        }
        if (!robot.IsStateWithinLimits(start) || !robot.IsStateWithinLimits(goal))
        {
            throw std::invalid_argument("Planner input state violates joint limits.");
        }
        if (sampleCount < 2)
        {
            throw std::invalid_argument("Planner requires at least 2 samples.");
        }
        if (durationSeconds <= 0.0)
        {
            throw std::invalid_argument("Planner duration must be positive.");
        }

        TrajectoryPlan plan;
        plan.Samples.reserve(sampleCount);
        const double timeStep = durationSeconds / static_cast<double>(sampleCount - 1);

        for (std::size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
        {
            const double t = static_cast<double>(sampleIndex) / static_cast<double>(sampleCount - 1);
            apex::core::JointState state = Interpolate(start, goal, t);
            const auto collisionReport = world.EvaluateRobotState(robot, state, linkSafetyRadius);
            if (collisionReport.HasCollision)
            {
                plan.IsCompletelyCollisionFree = false;
            }

            plan.Samples.push_back({
                timeStep * static_cast<double>(sampleIndex),
                std::move(state),
                robot.ComputeTcpPose(plan.Samples.empty() ? start : Interpolate(start, goal, t)),
                collisionReport.HasCollision });
        }

        return plan;
    }
}
