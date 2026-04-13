#pragma once

#include "apex/analysis/TrajectoryAnalyzer.h"
#include "apex/extension/ProjectAuditPlugin.h"
#include "apex/io/StudioProject.h"
#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/simulation/JobSimulationEngine.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::report
{
    class ProjectReportGenerator
    {
    public:
        [[nodiscard]] std::string BuildHtmlReport(
            const apex::io::StudioProject& project,
            const apex::planning::TrajectoryPlan& plan,
            const apex::analysis::TrajectoryQualityReport& qualityReport,
            const std::string& embeddedSvg) const;

        [[nodiscard]] std::string BuildJobHtmlReport(
            const apex::io::StudioProject& project,
            const apex::simulation::JobSimulationResult& simulationResult,
            const std::vector<apex::extension::PluginFinding>& findings,
            const std::string& embeddedSvg) const;

        void SaveHtmlReport(
            const apex::io::StudioProject& project,
            const apex::planning::TrajectoryPlan& plan,
            const apex::analysis::TrajectoryQualityReport& qualityReport,
            const std::string& embeddedSvg,
            const std::filesystem::path& filePath) const;

        void SaveJobHtmlReport(
            const apex::io::StudioProject& project,
            const apex::simulation::JobSimulationResult& simulationResult,
            const std::vector<apex::extension::PluginFinding>& findings,
            const std::string& embeddedSvg,
            const std::filesystem::path& filePath) const;
    };
}
