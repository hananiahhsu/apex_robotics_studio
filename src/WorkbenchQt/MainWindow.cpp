#include "MainWindow.h"

#include "apex/catalog/ProjectCatalog.h"
#include "apex/dashboard/WorkbenchDashboard.h"
#include "apex/delivery/DeliveryDossier.h"
#include "apex/importer/UrdfImporter.h"
#include "apex/importer/RobotDescriptionInspector.h"
#include "apex/integration/Ros2WorkspaceExport.h"
#include "apex/integration/IntegrationExport.h"
#include "apex/quality/ReleaseGate.h"
#include "apex/revision/ProjectSnapshot.h"
#include "apex/io/StudioProjectSerializer.h"
#include "apex/session/ExecutionSession.h"
#include "apex/platform/RuntimeConfig.h"
#include "apex/planning/JointTrajectoryPlanner.h"
#include "apex/simulation/JobSimulationEngine.h"
#include "apex/visualization/SvgExporter.h"
#include "apex/workcell/CollisionWorld.h"
#include "apex/core/SerialRobotModel.h"

#include <QAction>
#include <QFileDialog>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextBrowser>
#include <QTextEdit>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <fstream>
#include <sstream>

namespace apex::qt
{
    namespace
    {
        apex::core::SerialRobotModel BuildRobot(const apex::io::StudioProject& project)
        {
            apex::core::SerialRobotModel robot(project.RobotName);
            for (const auto& joint : project.Joints)
            {
                robot.AddRevoluteJoint(joint);
            }
            return robot;
        }

        apex::workcell::CollisionWorld BuildWorld(const apex::io::StudioProject& project)
        {
            apex::workcell::CollisionWorld world;
            for (const auto& obstacle : project.Obstacles)
            {
                world.AddObstacle(obstacle);
            }
            return world;
        }

        QString ToQString(const std::filesystem::path& path)
        {
            return QString::fromStdString(path.string());
        }
    }

    MainWindow::MainWindow()
    {
        BuildUi();
        statusBar()->showMessage("Ready");
    }

    void MainWindow::BuildUi()
    {
        setWindowTitle("Apex Robotics Workstation");
        resize(1440, 900);

        auto* const pCentral = new QWidget(this);
        auto* const pLayout = new QVBoxLayout(pCentral);
        auto* const pSplitter = new QSplitter(pCentral);
        auto* const pRightSplitter = new QSplitter(Qt::Vertical, pSplitter);

        m_pProjectTree = new QTreeWidget(pSplitter);
        m_pProjectTree->setColumnCount(2);
        m_pProjectTree->setHeaderLabels({"Name", "Details"});
        m_pProjectTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_pProjectTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);

        m_pSummaryView = new QTextBrowser(pRightSplitter);
        m_pSummaryView->setOpenExternalLinks(true);

        m_pSegmentsTable = new QTableWidget(pRightSplitter);
        m_pSegmentsTable->setColumnCount(6);
        m_pSegmentsTable->setHorizontalHeaderLabels({"Segment", "Route", "Tag", "Duration (s)", "Path (m)", "Status"});
        m_pSegmentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_pSegmentsTable->verticalHeader()->setVisible(false);

        m_pMeshTable = new QTableWidget(pRightSplitter);
        m_pMeshTable->setColumnCount(5);
        m_pMeshTable->setHorizontalHeaderLabels({"Link", "Role", "URI", "Package", "Resolved Path"});
        m_pMeshTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_pMeshTable->verticalHeader()->setVisible(false);

        m_pArtifactsView = new QTextBrowser(pRightSplitter);
        m_pArtifactsView->setHtml("<h3>Artifacts</h3><p>No exported artifacts yet.</p>");

        m_pLogView = new QTextEdit(pRightSplitter);
        m_pLogView->setReadOnly(true);
        m_pLogView->setPlainText("Open an .arsproject or import a .urdf/.xacro file to start.\n");

        pRightSplitter->addWidget(m_pSummaryView);
        pRightSplitter->addWidget(m_pSegmentsTable);
        pRightSplitter->addWidget(m_pMeshTable);
        pRightSplitter->addWidget(m_pArtifactsView);
        pRightSplitter->addWidget(m_pLogView);
        pRightSplitter->setStretchFactor(0, 4);
        pRightSplitter->setStretchFactor(1, 2);
        pRightSplitter->setStretchFactor(2, 1);
        pRightSplitter->setStretchFactor(3, 1);
        pRightSplitter->setStretchFactor(4, 1);

        pSplitter->addWidget(m_pProjectTree);
        pSplitter->addWidget(pRightSplitter);
        pSplitter->setStretchFactor(0, 1);
        pSplitter->setStretchFactor(1, 3);
        pLayout->addWidget(pSplitter);
        setCentralWidget(pCentral);

        auto* const fileMenu = menuBar()->addMenu("&File");
        auto* const openProject = fileMenu->addAction("Open Project...");
        auto* const importDescription = fileMenu->addAction("Import URDF/Xacro...");
        auto* const saveProject = fileMenu->addAction("Save Project As...");
        auto* const exportSvg = fileMenu->addAction("Export SVG Preview...");
        auto* const inspectDescription = fileMenu->addAction("Inspect Robot Description...");
        auto* const exportRos2 = fileMenu->addAction("Export ROS2/MoveIt Workspace...");
        auto* const exportIntegration = fileMenu->addAction("Export Integration Package...");
        auto* const evaluateGate = fileMenu->addAction("Evaluate Release Gate...");
        auto* const createSnapshot = fileMenu->addAction("Create Snapshot...");
        auto* const exportDashboard = fileMenu->addAction("Export Dashboard...");
        auto* const exportSession = fileMenu->addAction("Export Session Artifacts...");
        auto* const exportDossier = fileMenu->addAction("Export Delivery Dossier...");
        fileMenu->addSeparator();
        auto* const exitAction = fileMenu->addAction("E&xit");

        auto* const runMenu = menuBar()->addMenu("&Run");
        auto* const runAnalysis = runMenu->addAction("Run Job Analysis");

        auto* const helpMenu = menuBar()->addMenu("&Help");
        auto* const aboutAction = helpMenu->addAction("About");

        auto* const toolbar = addToolBar("Main");
        toolbar->addAction(openProject);
        toolbar->addAction(importDescription);
        toolbar->addAction(saveProject);
        toolbar->addAction(runAnalysis);
        toolbar->addAction(exportSvg);
        toolbar->addAction(inspectDescription);
        toolbar->addAction(exportRos2);
        toolbar->addAction(exportIntegration);
        toolbar->addAction(evaluateGate);
        toolbar->addAction(createSnapshot);
        toolbar->addAction(exportDashboard);
        toolbar->addAction(exportSession);
        toolbar->addAction(exportDossier);

        connect(openProject, &QAction::triggered, this, &MainWindow::OnOpenProject);
        connect(importDescription, &QAction::triggered, this, &MainWindow::OnImportRobotDescription);
        connect(saveProject, &QAction::triggered, this, &MainWindow::OnSaveProject);
        connect(runAnalysis, &QAction::triggered, this, &MainWindow::OnRunAnalysis);
        connect(exportSvg, &QAction::triggered, this, &MainWindow::OnExportSvg);
        connect(exportDashboard, &QAction::triggered, this, &MainWindow::OnExportDashboard);
        connect(inspectDescription, &QAction::triggered, this, &MainWindow::OnInspectRobotDescription);
        connect(exportSession, &QAction::triggered, this, &MainWindow::OnExportSession);
        connect(exportDossier, &QAction::triggered, this, &MainWindow::OnExportDeliveryDossier);
        connect(exportRos2, &QAction::triggered, this, &MainWindow::OnExportRos2Workspace);
        connect(exportIntegration, &QAction::triggered, this, &MainWindow::OnExportIntegrationPackage);
        connect(evaluateGate, &QAction::triggered, this, &MainWindow::OnEvaluateGate);
        connect(createSnapshot, &QAction::triggered, this, &MainWindow::OnCreateSnapshot);
        connect(aboutAction, &QAction::triggered, this, &MainWindow::OnAbout);
        connect(exitAction, &QAction::triggered, this, &QWidget::close);
    }

    std::filesystem::path MainWindow::RequireProjectPath(const QString& caption, const QString& filter)
    {
        const QString filePath = QFileDialog::getOpenFileName(this, caption, QString(), filter);
        return filePath.isEmpty() ? std::filesystem::path{} : std::filesystem::path(filePath.toStdString());
    }

    void MainWindow::LoadProjectFile(const std::filesystem::path& filePath)
    {
        const apex::io::StudioProjectSerializer serializer;
        m_project = serializer.LoadFromFile(filePath);
        m_projectPath = filePath;
        AppendLog("Loaded project: " + QString::fromStdString(filePath.string()));
        RefreshProjectViews();
    }

    void MainWindow::LoadImportedDescription(const std::filesystem::path& filePath)
    {
        const apex::importer::UrdfImporter importer;
        auto result = importer.ImportFromFile(filePath);
        if (result.UsedXacro)
        {
            const std::filesystem::path expandedPath = filePath.parent_path() / (filePath.stem().string() + ".expanded.urdf");
            std::ofstream expanded(expandedPath);
            expanded << result.ExpandedRobotXml;
            result.Project.DescriptionSource.ExpandedDescriptionPath = expandedPath.string();
            AppendLog("Expanded Xacro to: " + QString::fromStdString(expandedPath.string()));
        }
        m_project = result.Project;
        m_projectPath.clear();
        AppendLog("Imported robot description: " + QString::fromStdString(filePath.string()));
        for (const std::string& warning : result.Warnings)
        {
            AppendLog("Warning: " + QString::fromStdString(warning));
        }
        RefreshProjectViews();
    }

    void MainWindow::RefreshProjectViews()
    {
        m_pProjectTree->clear();
        m_pSegmentsTable->setRowCount(0);
        if (m_pMeshTable != nullptr)
        {
            m_pMeshTable->setRowCount(0);
        }
        if (!m_project.has_value())
        {
            m_pSummaryView->setHtml("<h2>No project loaded</h2>");
            return;
        }

        const apex::catalog::ProjectCatalog catalog = apex::catalog::ProjectCatalogBuilder().Build(*m_project);
        std::vector<QTreeWidgetItem*> parentStack;
        for (const auto& item : catalog.Items)
        {
            auto* const treeItem = new QTreeWidgetItem({QString::fromStdString(item.Kind + ": " + item.Name), QString::fromStdString(item.Details)});
            if (item.Depth == 0)
            {
                m_pProjectTree->addTopLevelItem(treeItem);
            }
            else if (static_cast<std::size_t>(item.Depth - 1) < parentStack.size())
            {
                parentStack[static_cast<std::size_t>(item.Depth - 1)]->addChild(treeItem);
            }
            else
            {
                m_pProjectTree->addTopLevelItem(treeItem);
            }
            if (parentStack.size() <= static_cast<std::size_t>(item.Depth))
            {
                parentStack.resize(static_cast<std::size_t>(item.Depth + 1));
            }
            parentStack[static_cast<std::size_t>(item.Depth)] = treeItem;
        }
        m_pProjectTree->expandAll();
        if (m_pMeshTable != nullptr)
        {
            m_pMeshTable->setRowCount(static_cast<int>(m_project->MeshResources.size()));
            for (int row = 0; row < static_cast<int>(m_project->MeshResources.size()); ++row)
            {
                const auto& mesh = m_project->MeshResources[static_cast<std::size_t>(row)];
                m_pMeshTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(mesh.LinkName)));
                m_pMeshTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(mesh.Role)));
                m_pMeshTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(mesh.Uri)));
                m_pMeshTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(mesh.PackageName)));
                m_pMeshTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(mesh.ResolvedPath)));
            }
        }
        RefreshSimulationView();
    }

    void MainWindow::RefreshSimulationView()
    {
        if (!m_project.has_value())
        {
            return;
        }

        const auto robot = BuildRobot(*m_project);
        const auto world = BuildWorld(*m_project);
        const apex::platform::RuntimeConfig runtimeConfig;
        apex::simulation::JobSimulationEngine engine;
        apex::job::RobotJob job = m_project->Job;
        if (job.Empty())
        {
            job.Name = "Preview Motion";
            job.LinkSafetyRadius = m_project->Motion.LinkSafetyRadius;
            job.Waypoints.push_back({"Start", m_project->Motion.Start});
            job.Waypoints.push_back({"Goal", m_project->Motion.Goal});
            job.Segments.push_back({"PrimaryMotion", "Start", "Goal", m_project->Motion.SampleCount, m_project->Motion.DurationSeconds, "preview"});
        }
        const auto simulation = engine.Simulate(robot, world, job, runtimeConfig.PreferredJointVelocityLimitRadPerSec);

        apex::session::SessionRecorder session;
        session.SetTitle("Qt Workbench Session");
        session.SetProjectName(m_project->ProjectName);
        session.SetJobName(simulation.JobName);
        session.SetMetrics(simulation.TotalDurationSeconds, simulation.TotalPathLengthMeters, 0);
        session.AddEvent("workbench", "info", "Project loaded in Qt workbench.");
        if (!simulation.CollisionFree)
        {
            session.AddWarning("Simulation reported one or more segment collisions.");
            session.MarkStatus(apex::session::SessionStatus::Warning);
        }

        const apex::catalog::ProjectCatalog catalog = apex::catalog::ProjectCatalogBuilder().Build(*m_project);
        const auto svg = apex::visualization::SvgExporter().ExportTopView(*m_project);
        const std::string html = apex::dashboard::WorkbenchDashboardGenerator().BuildProjectDashboard(
            *m_project,
            catalog,
            simulation,
            {},
            session.GetSummary(),
            svg);
        m_pSummaryView->setHtml(QString::fromStdString(html));

        m_pSegmentsTable->setRowCount(static_cast<int>(simulation.SegmentResults.size()));
        for (int row = 0; row < static_cast<int>(simulation.SegmentResults.size()); ++row)
        {
            const auto& segment = simulation.SegmentResults[static_cast<std::size_t>(row)];
            m_pSegmentsTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(segment.Segment.Name)));
            m_pSegmentsTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(segment.Segment.StartWaypointName + " -> " + segment.Segment.GoalWaypointName)));
            m_pSegmentsTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(segment.Segment.ProcessTag)));
            m_pSegmentsTable->setItem(row, 3, new QTableWidgetItem(QString::number(segment.Quality.EstimatedCycleTimeSeconds, 'f', 3)));
            m_pSegmentsTable->setItem(row, 4, new QTableWidgetItem(QString::number(segment.Quality.PathLengthMeters, 'f', 3)));
            m_pSegmentsTable->setItem(row, 5, new QTableWidgetItem(segment.Quality.CollisionFree ? "OK" : "Collision"));
        }
        m_pArtifactsView->setHtml(QString("<h3>Imported Assets</h3><p><b>Source kind:</b> %1</p><p><b>Mesh resources:</b> %2</p><p><b>Description packages:</b> %3</p>")
            .arg(QString::fromStdString(m_project->DescriptionSource.SourceKind))
            .arg(static_cast<int>(m_project->MeshResources.size()))
            .arg(static_cast<int>(m_project->DescriptionSource.PackageDependencies.size())));
        statusBar()->showMessage(QString("Project ready: %1 joints, %2 obstacles, %3 meshes, %4 packages")
            .arg(static_cast<int>(m_project->Joints.size()))
            .arg(static_cast<int>(m_project->Obstacles.size()))
            .arg(static_cast<int>(m_project->MeshResources.size()))
            .arg(static_cast<int>(m_project->DescriptionSource.PackageDependencies.size())));
    }

    void MainWindow::AppendLog(const QString& message)
    {
        m_pLogView->append(message);
    }

    void MainWindow::OnOpenProject()
    {
        const auto filePath = RequireProjectPath("Open Apex Project", "Apex Project (*.arsproject)");
        if (!filePath.empty())
        {
            LoadProjectFile(filePath);
        }
    }

    void MainWindow::OnImportRobotDescription()
    {
        const auto filePath = RequireProjectPath("Import Robot Description", "Robot Description (*.urdf *.xacro)");
        if (!filePath.empty())
        {
            LoadImportedDescription(filePath);
        }
    }


    void MainWindow::OnSaveProject()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Save Project", "Load a project or import a robot description first.");
            return;
        }
        const QString filePath = QFileDialog::getSaveFileName(this, "Save Apex Project", m_projectPath.empty() ? "project.arsproject" : ToQString(m_projectPath), "Apex Project (*.arsproject)");
        if (filePath.isEmpty())
        {
            return;
        }
        const apex::io::StudioProjectSerializer serializer;
        serializer.SaveToFile(*m_project, filePath.toStdString());
        m_projectPath = filePath.toStdString();
        AppendLog("Project saved: " + filePath);
    }

    void MainWindow::OnRunAnalysis()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Run Analysis", "Load a project or import a robot description first.");
            return;
        }
        RefreshSimulationView();
        AppendLog("Job analysis re-ran successfully.");
    }


    void MainWindow::OnExportSvg()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Export SVG", "Load a project first.");
            return;
        }
        const QString filePath = QFileDialog::getSaveFileName(this, "Export SVG Preview", "workcell_preview.svg", "SVG (*.svg)");
        if (filePath.isEmpty())
        {
            return;
        }
        const auto robot = BuildRobot(*m_project);
        const auto world = BuildWorld(*m_project);
        const apex::visualization::SvgExporter exporter;
        if (!m_project->Job.Empty())
        {
            const apex::platform::RuntimeConfig runtimeConfig;
            apex::simulation::JobSimulationEngine engine;
            const auto simulation = engine.Simulate(robot, world, m_project->Job, runtimeConfig.PreferredJointVelocityLimitRadPerSec);
            exporter.SaveJobTopViewSvg(robot, world, simulation, filePath.toStdString(), {1200, 800, 60.0, m_project->ProjectName, true, true, true});
        }
        else
        {
            apex::planning::JointTrajectoryPlanner planner;
            const auto plan = planner.Plan(robot, m_project->Motion.Start, m_project->Motion.Goal, m_project->Motion.SampleCount, m_project->Motion.DurationSeconds);
            exporter.SaveProjectTopViewSvg(robot, world, plan, filePath.toStdString(), {1200, 800, 60.0, m_project->ProjectName, true, true, true});
        }
        if (m_pArtifactsView != nullptr)
        {
            m_pArtifactsView->setHtml("<h3>Artifacts</h3><ul><li>SVG preview: " + filePath + "</li></ul>");
        }
        AppendLog("SVG exported: " + filePath);
    }

    void MainWindow::OnExportDashboard()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Export Dashboard", "Load a project first.");
            return;
        }
        const QString filePath = QFileDialog::getSaveFileName(this, "Export Dashboard", "workstation_dashboard.html", "HTML (*.html)");
        if (filePath.isEmpty())
        {
            return;
        }
        std::ofstream stream(filePath.toStdString());
        stream << m_pSummaryView->toHtml().toStdString();
        if (m_pArtifactsView != nullptr)
        {
            m_pArtifactsView->setHtml("<h3>Artifacts</h3><ul><li>Dashboard: " + filePath + "</li></ul>");
        }
        AppendLog("Dashboard exported: " + filePath);
    }


    void MainWindow::OnExportSession()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Export Session", "Load a project first.");
            return;
        }
        const QString directory = QFileDialog::getExistingDirectory(this, "Choose Session Output Directory", QString());
        if (directory.isEmpty())
        {
            return;
        }
        const auto robot = BuildRobot(*m_project);
        const auto world = BuildWorld(*m_project);
        const apex::platform::RuntimeConfig runtimeConfig;
        apex::simulation::JobSimulationEngine engine;
        apex::job::RobotJob job = m_project->Job;
        if (job.Empty())
        {
            job.Name = "Preview Motion";
            job.LinkSafetyRadius = m_project->Motion.LinkSafetyRadius;
            job.Waypoints.push_back({"Start", m_project->Motion.Start});
            job.Waypoints.push_back({"Goal", m_project->Motion.Goal});
            job.Segments.push_back({"PrimaryMotion", "Start", "Goal", m_project->Motion.SampleCount, m_project->Motion.DurationSeconds, "preview"});
        }
        const auto simulation = engine.Simulate(robot, world, job, runtimeConfig.PreferredJointVelocityLimitRadPerSec);
        apex::session::SessionRecorder session;
        session.SetTitle("Qt Workbench Export Session");
        session.SetProjectName(m_project->ProjectName);
        session.SetJobName(simulation.JobName);
        session.SetMetrics(simulation.TotalDurationSeconds, simulation.TotalPathLengthMeters, 0);
        session.AddEvent("workbench", "info", "Exported from Qt workbench.");
        if (!simulation.CollisionFree)
        {
            session.AddWarning("Simulation reported one or more segment collisions.");
            session.MarkStatus(apex::session::SessionStatus::Warning);
        }
        session.SaveToDirectory(directory.toStdString());
        AppendLog("Session exported: " + directory);
    }

    void MainWindow::OnInspectRobotDescription()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Inspect Robot Description", "Load a project or import a robot description first.");
            return;
        }
        const QString filePath = QFileDialog::getSaveFileName(this, "Export Robot Description Inspection", "robot_description_inspection.html", "HTML (*.html);;JSON (*.json)");
        if (filePath.isEmpty())
        {
            return;
        }
        const apex::importer::RobotDescriptionInspector inspector;
        const auto inspection = inspector.Inspect(*m_project);
        const std::filesystem::path outputPath(filePath.toStdString());
        if (outputPath.extension() == ".json")
        {
            inspector.WriteJsonReport(inspection, outputPath);
        }
        else
        {
            inspector.WriteHtmlReport(inspection, outputPath);
        }
        AppendLog("Robot description inspection exported: " + filePath);
        if (m_pArtifactsView != nullptr)
        {
            m_pArtifactsView->setHtml("<h3>Artifacts</h3><ul><li>Robot description inspection: " + filePath + "</li></ul>");
        }
    }

    void MainWindow::OnExportRos2Workspace()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Export ROS2 Workspace", "Load a project or import a robot description first.");
            return;
        }
        const QString directory = QFileDialog::getExistingDirectory(this, "Choose Output Directory", QString());
        if (directory.isEmpty())
        {
            return;
        }
        const apex::integration::Ros2WorkspaceExporter exporter;
        const auto result = exporter.ExportWorkspace(*m_project, directory.toStdString());
        AppendLog("ROS2 workspace exported: " + QString::fromStdString(result.WorkspaceRoot.string()));
        QMessageBox::information(this, "ROS2/MoveIt Export",
            "Workspace exported to:\n" + ToQString(result.WorkspaceRoot) +
            "\n\nBringup package:\n" + ToQString(result.BringupPackageRoot) +
            "\n\nMoveIt skeleton:\n" + ToQString(result.MoveItPackageRoot));
    }

    void MainWindow::OnExportDeliveryDossier()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Export Delivery Dossier", "Load a project first.");
            return;
        }
        const QString directory = QFileDialog::getExistingDirectory(this, "Export Delivery Dossier", QString(), QFileDialog::ShowDirsOnly);
        if (directory.isEmpty())
        {
            return;
        }
        apex::delivery::DeliveryDossierBuilder builder;
        const apex::platform::RuntimeConfig runtimeConfig;
        builder.Build(*m_project, directory.toStdString(), runtimeConfig, {});
        if (m_pArtifactsView != nullptr)
        {
            m_pArtifactsView->setHtml("<h3>Artifacts</h3><ul><li>Delivery dossier: " + directory + "</li></ul>");
        }
        AppendLog("Delivery dossier exported: " + directory);
    }


    void MainWindow::OnExportIntegrationPackage()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Export Integration Package", "Load a project first.");
            return;
        }
        const QString directory = QFileDialog::getExistingDirectory(this, "Export Integration Package", QString());
        if (directory.isEmpty())
        {
            return;
        }
        const auto robot = BuildRobot(*m_project);
        const auto world = BuildWorld(*m_project);
        const apex::platform::RuntimeConfig runtimeConfig;
        apex::simulation::JobSimulationEngine engine;
        apex::job::RobotJob job = m_project->Job;
        if (job.Empty())
        {
            job.Name = "Preview Motion";
            job.LinkSafetyRadius = m_project->Motion.LinkSafetyRadius;
            job.Waypoints.push_back({"Start", m_project->Motion.Start});
            job.Waypoints.push_back({"Goal", m_project->Motion.Goal});
            job.Segments.push_back({"PrimaryMotion", "Start", "Goal", m_project->Motion.SampleCount, m_project->Motion.DurationSeconds, "preview"});
        }
        const auto simulation = engine.Simulate(robot, world, job, runtimeConfig.PreferredJointVelocityLimitRadPerSec);
        const std::vector<apex::extension::PluginFinding> findings;
        const apex::quality::GateEvaluation gate = apex::quality::ReleaseGateEvaluator().Evaluate(*m_project, simulation, findings);
        apex::integration::IntegrationPackageExporter exporter;
        exporter.Export(*m_project, simulation, gate, findings, directory.toStdString(), nullptr);
        AppendLog("Integration package exported: " + directory);
        if (m_pArtifactsView != nullptr)
        {
            m_pArtifactsView->setHtml("<h3>Artifacts</h3><ul><li>Integration package: " + directory + "</li></ul>");
        }
    }

    void MainWindow::OnEvaluateGate()
    {
        if (!m_project.has_value())
        {
            QMessageBox::warning(this, "Evaluate Release Gate", "Load a project first.");
            return;
        }
        const QString filePath = QFileDialog::getSaveFileName(this, "Export Release Gate Report", "release_gate.html", "HTML (*.html)");
        if (filePath.isEmpty())
        {
            return;
        }
        const auto robot = BuildRobot(*m_project);
        const auto world = BuildWorld(*m_project);
        const apex::platform::RuntimeConfig runtimeConfig;
        apex::simulation::JobSimulationEngine engine;
        apex::job::RobotJob job = m_project->Job;
        if (job.Empty())
        {
            job.Name = "Preview Motion";
            job.LinkSafetyRadius = m_project->Motion.LinkSafetyRadius;
            job.Waypoints.push_back({"Start", m_project->Motion.Start});
            job.Waypoints.push_back({"Goal", m_project->Motion.Goal});
            job.Segments.push_back({"PrimaryMotion", "Start", "Goal", m_project->Motion.SampleCount, m_project->Motion.DurationSeconds, "preview"});
        }
        const auto simulation = engine.Simulate(robot, world, job, runtimeConfig.PreferredJointVelocityLimitRadPerSec);
        const std::vector<apex::extension::PluginFinding> findings;
        apex::quality::ReleaseGateEvaluator evaluator;
        const auto evaluation = evaluator.Evaluate(*m_project, simulation, findings);
        evaluator.SaveHtmlReport(*m_project, evaluation, filePath.toStdString());
        AppendLog("Release gate report exported: " + filePath);
    }

    void MainWindow::OnCreateSnapshot()
    {
        if (!m_project.has_value() || m_projectPath.empty())
        {
            QMessageBox::warning(this, "Create Snapshot", "Save the current project to a file first.");
            return;
        }
        const QString directory = QFileDialog::getExistingDirectory(this, "Create Project Snapshot", QString());
        if (directory.isEmpty())
        {
            return;
        }
        apex::revision::ProjectSnapshotManager manager;
        const auto manifest = manager.CreateSnapshot(m_projectPath, directory.toStdString(), nullptr, nullptr);
        manager.SaveManifestJson(manifest, std::filesystem::path(directory.toStdString()) / "snapshot_manifest.json");
        manager.SaveIndexHtml(manifest, std::filesystem::path(directory.toStdString()) / "index.html");
        AppendLog("Project snapshot created: " + directory);
    }

    void MainWindow::OnAbout()
    {
        QMessageBox::information(this, "About Apex Robotics Workstation",
            "Apex Robotics Workstation v1.3 source upgrade\n\n"
            "This Qt frontend can load .arsproject files, import URDF/Xacro robot descriptions, "
            "run job analysis, render a dashboard, and export a ROS2/MoveIt workspace skeleton.");
    }
}
