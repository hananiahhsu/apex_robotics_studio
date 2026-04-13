#pragma once

#include "apex/io/StudioProject.h"
#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/simulation/JobSimulationEngine.h"
#include "apex/workcell/CollisionWorld.h"

#include <filesystem>
#include <string>

namespace apex::visualization
{
    struct SvgExportOptions
    {
        int Width = 1200;
        int Height = 800;
        double MarginPixels = 60.0;
        std::string Title = "Apex Robotics Studio Workcell";
        bool ShowTcpPath = true;
        bool ShowRobotAtStart = true;
        bool ShowRobotAtGoal = true;
    };

    class SvgExporter
    {
    public:
        [[nodiscard]] std::string BuildProjectTopViewSvg(
            const apex::core::SerialRobotModel& robot,
            const apex::workcell::CollisionWorld& world,
            const apex::planning::TrajectoryPlan& plan,
            const SvgExportOptions& options = {}) const;

        [[nodiscard]] std::string BuildJobTopViewSvg(
            const apex::core::SerialRobotModel& robot,
            const apex::workcell::CollisionWorld& world,
            const apex::simulation::JobSimulationResult& result,
            const SvgExportOptions& options = {}) const;

        void SaveProjectTopViewSvg(
            const apex::core::SerialRobotModel& robot,
            const apex::workcell::CollisionWorld& world,
            const apex::planning::TrajectoryPlan& plan,
            const std::filesystem::path& filePath,
            const SvgExportOptions& options = {}) const;

        void SaveJobTopViewSvg(
            const apex::core::SerialRobotModel& robot,
            const apex::workcell::CollisionWorld& world,
            const apex::simulation::JobSimulationResult& result,
            const std::filesystem::path& filePath,
            const SvgExportOptions& options = {}) const;
    };
}
