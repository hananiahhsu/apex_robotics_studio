#include "apex/workcell/CollisionWorld.h"

#include <algorithm>
#include <stdexcept>

namespace apex::workcell
{
    void CollisionWorld::AddObstacle(const Obstacle& obstacle)
    {
        if (obstacle.Bounds.Min.X > obstacle.Bounds.Max.X ||
            obstacle.Bounds.Min.Y > obstacle.Bounds.Max.Y ||
            obstacle.Bounds.Min.Z > obstacle.Bounds.Max.Z)
        {
            throw std::invalid_argument("Obstacle bounds are invalid.");
        }

        m_obstacles.push_back(obstacle);
    }

    const std::vector<Obstacle>& CollisionWorld::GetObstacles() const noexcept
    {
        return m_obstacles;
    }

    CollisionReport CollisionWorld::EvaluateRobotState(
        const apex::core::SerialRobotModel& robot,
        const apex::core::JointState& state,
        double linkSafetyRadius) const
    {
        if (linkSafetyRadius <= 0.0)
        {
            throw std::invalid_argument("Link safety radius must be positive.");
        }

        CollisionReport report;
        const std::vector<apex::core::Vec3> jointPositions = robot.ComputeJointPositions(state);
        for (std::size_t linkIndex = 1; linkIndex < jointPositions.size(); ++linkIndex)
        {
            const apex::core::Vec3 center = jointPositions[linkIndex];
            for (const Obstacle& obstacle : m_obstacles)
            {
                apex::core::Vec3 closestPoint{};
                if (IntersectsSphereAabb(center, linkSafetyRadius, obstacle.Bounds, closestPoint))
                {
                    report.HasCollision = true;
                    report.Events.push_back({ obstacle.Name, linkIndex - 1, closestPoint });
                }
            }
        }
        return report;
    }

    apex::core::Vec3 CollisionWorld::ClampPointToAabb(const apex::core::Vec3& point, const Aabb& bounds) noexcept
    {
        return {
            std::clamp(point.X, bounds.Min.X, bounds.Max.X),
            std::clamp(point.Y, bounds.Min.Y, bounds.Max.Y),
            std::clamp(point.Z, bounds.Min.Z, bounds.Max.Z)
        };
    }

    bool CollisionWorld::IntersectsSphereAabb(
        const apex::core::Vec3& center,
        double radius,
        const Aabb& bounds,
        apex::core::Vec3& outClosestPoint) noexcept
    {
        outClosestPoint = ClampPointToAabb(center, bounds);
        const apex::core::Vec3 delta = center - outClosestPoint;
        return apex::core::NormSquared(delta) <= radius * radius;
    }
}
