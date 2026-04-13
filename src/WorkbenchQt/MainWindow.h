#pragma once

#include "apex/io/StudioProject.h"

#include <QMainWindow>

#include <filesystem>
#include <optional>

QT_BEGIN_NAMESPACE
class QTreeWidget;
class QTextBrowser;
class QTextEdit;
class QTableWidget;
QT_END_NAMESPACE

namespace apex::qt
{
    class MainWindow final : public QMainWindow
    {
        Q_OBJECT
    public:
        MainWindow();
    private:
        void BuildUi();
        void LoadProjectFile(const std::filesystem::path& filePath);
        void LoadImportedDescription(const std::filesystem::path& filePath);
        void RefreshProjectViews();
        void RefreshSimulationView();
        void AppendLog(const QString& message);
        std::filesystem::path RequireProjectPath(const QString& caption, const QString& filter);

    private slots:
        void OnOpenProject();
        void OnImportRobotDescription();
        void OnSaveProject();
        void OnRunAnalysis();
        void OnExportSvg();
        void OnExportDashboard();
        void OnExportSession();
        void OnInspectRobotDescription();
        void OnExportRos2Workspace();
        void OnExportIntegrationPackage();
        void OnEvaluateGate();
        void OnCreateSnapshot();
        void OnExportDeliveryDossier();
        void OnAbout();

    private:
        std::optional<apex::io::StudioProject> m_project;
        std::filesystem::path m_projectPath;
        QTreeWidget* m_pProjectTree = nullptr;
        QTextBrowser* m_pSummaryView = nullptr;
        QTextEdit* m_pLogView = nullptr;
        QTableWidget* m_pSegmentsTable = nullptr;
        QTableWidget* m_pMeshTable = nullptr;
        QTextBrowser* m_pArtifactsView = nullptr;
    };
}
