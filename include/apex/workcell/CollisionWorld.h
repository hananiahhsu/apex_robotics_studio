#pragma once

#include "apex/core/MathTypes.h"
#include "apex/core/SerialRobotModel.h"

#include <string>
#include <vector>

namespace apex::workcell
{
    struct Aabb
    {
        apex::core::Vec3 Min;
        apex::core::Vec3 Max;
    };

    struct Obstacle
    {
        std::string Name;
        Aabb Bounds;
    };

    struct CollisionEvent
    {
        std::string ObstacleName;
        std::size_t LinkIndex = 0;
        apex::core::Vec3 ClosestPoint;
    };

    struct CollisionReport
    {
        bool HasCollision = false;
        std::vector<CollisionEvent> Events;
    };

    class CollisionWorld
    {
    public:
        void AddObstacle(const Obstacle& obstacle);
        [[nodiscard]] const std::vector<Obstacle>& GetObstacles() const noexcept;
        [[nodiscard]] CollisionReport EvaluateRobotState(
            const apex::core::SerialRobotModel& robot,
            const apex::core::JointState& state,
            double linkSafetyRadius) const;

    private:
        [[nodiscard]] static apex::core::Vec3 ClampPointToAabb(const apex::core::Vec3& point, const Aabb& bounds) noexcept;
        [[nodiscard]] static bool IntersectsSphereAabb(
            const apex::core::Vec3& center,
            double radius,
            const Aabb& bounds,
            apex::core::Vec3& outClosestPoint) noexcept;

    private:
        std::vector<Obstacle> m_obstacles;
    };
}
