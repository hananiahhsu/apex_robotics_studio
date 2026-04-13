#pragma once

#include "apex/core/MathTypes.h"
#include "apex/core/RobotTypes.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace apex::core
{
    class SerialRobotModel
    {
    public:
        explicit SerialRobotModel(std::string name);

        void AddRevoluteJoint(const JointDefinition& joint);
        [[nodiscard]] const std::string& GetName() const noexcept;
        [[nodiscard]] std::size_t GetJointCount() const noexcept;
        [[nodiscard]] const std::vector<JointDefinition>& GetJoints() const noexcept;

        [[nodiscard]] bool IsStateDimensionValid(const JointState& state) const noexcept;
        [[nodiscard]] bool IsStateWithinLimits(const JointState& state) const noexcept;

        [[nodiscard]] std::vector<Vec3> ComputeJointPositions(const JointState& state) const;
        [[nodiscard]] TcpPose ComputeTcpPose(const JointState& state) const;

    private:
        void ValidateState(const JointState& state) const;

    private:
        std::string m_name;
        std::vector<JointDefinition> m_joints;
    };
}
