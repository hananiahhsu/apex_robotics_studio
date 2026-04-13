#include "apex/core/SerialRobotModel.h"

#include <cmath>

namespace apex::core
{
    SerialRobotModel::SerialRobotModel(std::string name)
        : m_name(std::move(name))
    {
    }

    void SerialRobotModel::AddRevoluteJoint(const JointDefinition& joint)
    {
        if (joint.LinkLength <= 0.0)
        {
            throw std::invalid_argument("Link length must be positive.");
        }
        if (joint.MinAngleRad > joint.MaxAngleRad)
        {
            throw std::invalid_argument("Joint min angle cannot exceed max angle.");
        }
        m_joints.push_back(joint);
    }

    const std::string& SerialRobotModel::GetName() const noexcept
    {
        return m_name;
    }

    std::size_t SerialRobotModel::GetJointCount() const noexcept
    {
        return m_joints.size();
    }

    const std::vector<JointDefinition>& SerialRobotModel::GetJoints() const noexcept
    {
        return m_joints;
    }

    bool SerialRobotModel::IsStateDimensionValid(const JointState& state) const noexcept
    {
        return state.JointAnglesRad.size() == m_joints.size();
    }

    bool SerialRobotModel::IsStateWithinLimits(const JointState& state) const noexcept
    {
        if (!IsStateDimensionValid(state))
        {
            return false;
        }

        for (std::size_t index = 0; index < m_joints.size(); ++index)
        {
            const double angle = state.JointAnglesRad[index];
            const JointDefinition& joint = m_joints[index];
            if (angle < joint.MinAngleRad || angle > joint.MaxAngleRad)
            {
                return false;
            }
        }
        return true;
    }

    std::vector<Vec3> SerialRobotModel::ComputeJointPositions(const JointState& state) const
    {
        ValidateState(state);

        std::vector<Vec3> points;
        points.reserve(m_joints.size() + 1);
        points.push_back({ 0.0, 0.0, 0.0 });

        double accumulatedAngle = 0.0;
        Vec3 cursor{ 0.0, 0.0, 0.0 };
        for (std::size_t index = 0; index < m_joints.size(); ++index)
        {
            accumulatedAngle += state.JointAnglesRad[index];
            cursor.X += std::cos(accumulatedAngle) * m_joints[index].LinkLength;
            cursor.Y += std::sin(accumulatedAngle) * m_joints[index].LinkLength;
            points.push_back(cursor);
        }

        return points;
    }

    TcpPose SerialRobotModel::ComputeTcpPose(const JointState& state) const
    {
        ValidateState(state);
        const std::vector<Vec3> points = ComputeJointPositions(state);
        const Vec3& tcp = points.back();

        double heading = 0.0;
        for (double angle : state.JointAnglesRad)
        {
            heading += angle;
        }

        return { tcp.X, tcp.Y, tcp.Z, heading };
    }

    void SerialRobotModel::ValidateState(const JointState& state) const
    {
        if (!IsStateDimensionValid(state))
        {
            throw std::invalid_argument("Joint state dimension does not match the robot model.");
        }
        if (!IsStateWithinLimits(state))
        {
            throw std::invalid_argument("Joint state violates robot joint limits.");
        }
    }
}
