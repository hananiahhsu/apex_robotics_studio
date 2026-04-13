#include "apex/analysis/TrajectoryAnalyzer.h"

#include "apex/core/MathTypes.h"

#include <algorithm>

namespace apex::analysis
{
    TrajectoryQualityReport TrajectoryAnalyzer::Analyze(
        const apex::core::SerialRobotModel& robot,
        const apex::workcell::CollisionWorld& world,
        const apex::planning::TrajectoryPlan& plan,
        double linkSafetyRadius,
        double preferredJointVelocityLimitRadPerSec) const
    {
        TrajectoryQualityReport report;
        if (plan.Samples.empty())
        {
            report.Warnings.push_back("Trajectory plan contains no samples.");
            return report;
        }

        report.EstimatedCycleTimeSeconds = plan.Samples.back().TimeSeconds;
        report.JointVelocityMetrics.resize(robot.GetJointCount());
        for (std::size_t index = 0; index < robot.GetJointCount(); ++index)
        {
            report.JointVelocityMetrics[index].JointName = robot.GetJoints()[index].Name;
            report.JointVelocityMetrics[index].PreferredVelocityLimitRadPerSec = preferredJointVelocityLimitRadPerSec;
        }

        for (std::size_t sampleIndex = 0; sampleIndex < plan.Samples.size(); ++sampleIndex)
        {
            const auto& sample = plan.Samples[sampleIndex];
            const auto collisionReport = world.EvaluateRobotState(robot, sample.State, linkSafetyRadius);
            if (collisionReport.HasCollision)
            {
                report.CollisionFree = false;
                ++report.CollisionSampleCount;
                if (report.FirstCollisionSampleIndex == static_cast<std::size_t>(-1))
                {
                    report.FirstCollisionSampleIndex = sampleIndex;
                }
            }

            if (sampleIndex > 0)
            {
                const auto& previous = plan.Samples[sampleIndex - 1];
                const apex::core::Vec3 delta{
                    sample.Tcp.X - previous.Tcp.X,
                    sample.Tcp.Y - previous.Tcp.Y,
                    sample.Tcp.Z - previous.Tcp.Z };
                const double stepDistance = apex::core::Norm(delta);
                report.PathLengthMeters += stepDistance;
                report.MaxTcpStepMeters = std::max(report.MaxTcpStepMeters, stepDistance);

                const double timeDelta = sample.TimeSeconds - previous.TimeSeconds;
                if (timeDelta > 0.0)
                {
                    const double tcpSpeed = stepDistance / timeDelta;
                    report.PeakTcpSpeedMetersPerSecond = std::max(report.PeakTcpSpeedMetersPerSecond, tcpSpeed);
                    for (std::size_t jointIndex = 0; jointIndex < sample.State.JointAnglesRad.size(); ++jointIndex)
                    {
                        const double velocity = std::abs(sample.State.JointAnglesRad[jointIndex] - previous.State.JointAnglesRad[jointIndex]) / timeDelta;
                        auto& metric = report.JointVelocityMetrics[jointIndex];
                        metric.MaxAbsVelocityRadPerSec = std::max(metric.MaxAbsVelocityRadPerSec, velocity);
                        if (velocity > preferredJointVelocityLimitRadPerSec)
                        {
                            metric.ExceedsPreferredVelocityLimit = true;
                        }
                    }
                }
            }
        }

        if (report.EstimatedCycleTimeSeconds > 0.0)
        {
            report.AverageTcpSpeedMetersPerSecond = report.PathLengthMeters / report.EstimatedCycleTimeSeconds;
        }
        if (!report.CollisionFree)
        {
            report.Warnings.push_back("Trajectory collides with the workcell.");
        }
        if (report.MaxTcpStepMeters > 0.20)
        {
            report.Warnings.push_back("TCP sample spacing is coarse; consider increasing sample count.");
        }
        for (const auto& metric : report.JointVelocityMetrics)
        {
            if (metric.ExceedsPreferredVelocityLimit)
            {
                report.Warnings.push_back("Joint '" + metric.JointName + "' exceeds preferred velocity limit.");
            }
        }
        return report;
    }
}
