#pragma once

#include "apex/catalog/ProjectCatalog.h"
#include "apex/extension/ProjectAuditPlugin.h"
#include "apex/io/StudioProject.h"
#include "apex/ops/BatchRunner.h"
#include "apex/session/ExecutionSession.h"
#include "apex/simulation/JobSimulationEngine.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::dashboard
{
    class WorkbenchDashboardGenerator
    {
    public:
        [[nodiscard]] std::string BuildProjectDashboard(
            const apex::io::StudioProject& project,
            const apex::catalog::ProjectCatalog& catalog,
            const apex::simulation::JobSimulationResult& simulationResult,
            const std::vector<apex::extension::PluginFinding>& findings,
            const apex::session::ExecutionSessionSummary& sessionSummary,
            const std::string& embeddedSvg) const;

        [[nodiscard]] std::string BuildBatchDashboard(
            const apex::ops::BatchRunResult& batchResult,
            const apex::session::ExecutionSessionSummary& sessionSummary) const;

        void SaveProjectDashboard(
            const apex::io::StudioProject& project,
            const apex::catalog::ProjectCatalog& catalog,
            const apex::simulation::JobSimulationResult& simulationResult,
            const std::vector<apex::extension::PluginFinding>& findings,
            const apex::session::ExecutionSessionSummary& sessionSummary,
            const std::string& embeddedSvg,
            const std::filesystem::path& filePath) const;

        void SaveBatchDashboard(
            const apex::ops::BatchRunResult& batchResult,
            const apex::session::ExecutionSessionSummary& sessionSummary,
            const std::filesystem::path& filePath) const;
    };
}
