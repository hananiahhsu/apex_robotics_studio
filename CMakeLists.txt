cmake_minimum_required(VERSION 3.24)

project(ApexRoboticsStudio
    VERSION 1.7.0
    DESCRIPTION "Cross-platform robotics workstation interview project"
    LANGUAGES CXX)

option(ARS_ENABLE_TESTS "Build unit tests" ON)
option(ARS_STRICT_WARNINGS "Enable strict compiler warnings" ON)
option(ARS_BUILD_APP "Build the CLI application" ON)
option(ARS_ENABLE_ROS2_BRIDGE "Enable ROS 2 bridge integration helpers" OFF)
option(ARS_ENABLE_QT_FRONTEND "Build optional Qt workstation frontend" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerOptions)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(ARS_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(ARS_INSTALL_SAMPLESDIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/samples")

add_library(ars_core
    src/Core/SerialRobotModel.cpp)
add_library(Apex::Core ALIAS ars_core)
set_target_properties(ars_core PROPERTIES
    OUTPUT_NAME apex_robotics_studio_core
    EXPORT_NAME Core
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ars_apply_common_options(ars_core)

add_library(ars_workcell
    src/Workcell/CollisionWorld.cpp)
add_library(Apex::Workcell ALIAS ars_workcell)
set_target_properties(ars_workcell PROPERTIES
    OUTPUT_NAME apex_robotics_studio_workcell
    EXPORT_NAME Workcell
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_workcell
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_workcell
    PUBLIC
        Apex::Core)
ars_apply_common_options(ars_workcell)

add_library(ars_planning
    src/Planning/JointTrajectoryPlanner.cpp)
add_library(Apex::Planning ALIAS ars_planning)
set_target_properties(ars_planning PROPERTIES
    OUTPUT_NAME apex_robotics_studio_planning
    EXPORT_NAME Planning
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_planning
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_planning
    PUBLIC
        Apex::Core
        Apex::Workcell)
ars_apply_common_options(ars_planning)

add_library(ars_job
    src/Job/RobotJob.cpp)
add_library(Apex::Job ALIAS ars_job)
set_target_properties(ars_job PROPERTIES
    OUTPUT_NAME apex_robotics_studio_job
    EXPORT_NAME Job
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_job
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_job
    PUBLIC
        Apex::Core)
ars_apply_common_options(ars_job)

add_library(ars_io
    src/IO/StudioProjectSerializer.cpp)
add_library(Apex::IO ALIAS ars_io)
set_target_properties(ars_io PROPERTIES
    OUTPUT_NAME apex_robotics_studio_io
    EXPORT_NAME IO
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_io
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_io
    PUBLIC
        Apex::Core
        Apex::Workcell
        Apex::Job)
ars_apply_common_options(ars_io)

add_library(ars_importer
    src/Importer/UrdfImporter.cpp
    src/Importer/RobotDescriptionInspector.cpp)
add_library(Apex::Importer ALIAS ars_importer)
set_target_properties(ars_importer PROPERTIES
    OUTPUT_NAME apex_robotics_studio_importer
    EXPORT_NAME Importer
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_importer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_importer
    PUBLIC
        Apex::Core
        Apex::IO)
ars_apply_common_options(ars_importer)

add_library(ars_analysis
    src/Analysis/TrajectoryAnalyzer.cpp)
add_library(Apex::Analysis ALIAS ars_analysis)
set_target_properties(ars_analysis PROPERTIES
    OUTPUT_NAME apex_robotics_studio_analysis
    EXPORT_NAME Analysis
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_analysis
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_analysis
    PUBLIC
        Apex::Core
        Apex::Workcell
        Apex::Planning)
ars_apply_common_options(ars_analysis)

add_library(ars_simulation
    src/Simulation/JobSimulationEngine.cpp)
add_library(Apex::Simulation ALIAS ars_simulation)
set_target_properties(ars_simulation PROPERTIES
    OUTPUT_NAME apex_robotics_studio_simulation
    EXPORT_NAME Simulation
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_simulation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_simulation
    PUBLIC
        Apex::Core
        Apex::Workcell
        Apex::Planning
        Apex::Analysis
        Apex::Job)
ars_apply_common_options(ars_simulation)

add_library(ars_extension
    src/Extension/ProjectAuditPlugin.cpp
    src/Extension/ExternalAuditPlugin.cpp)
add_library(Apex::Extension ALIAS ars_extension)
set_target_properties(ars_extension PROPERTIES
    OUTPUT_NAME apex_robotics_studio_extension
    EXPORT_NAME Extension
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_extension
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_extension
    PUBLIC
        Apex::Core
        Apex::Workcell
        Apex::IO)
ars_apply_common_options(ars_extension)

add_library(ars_platform
    src/Platform/RuntimeConfig.cpp)
add_library(Apex::Platform ALIAS ars_platform)
set_target_properties(ars_platform PROPERTIES
    OUTPUT_NAME apex_robotics_studio_platform
    EXPORT_NAME Platform
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_platform
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ars_apply_common_options(ars_platform)

add_library(ars_bundle
    src/Bundle/ProjectBundle.cpp)
add_library(Apex::Bundle ALIAS ars_bundle)
set_target_properties(ars_bundle PROPERTIES
    OUTPUT_NAME apex_robotics_studio_bundle
    EXPORT_NAME Bundle
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_bundle
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ars_apply_common_options(ars_bundle)

add_library(ars_ops
    src/Ops/BatchRunner.cpp)
add_library(Apex::Ops ALIAS ars_ops)
set_target_properties(ars_ops PROPERTIES
    OUTPUT_NAME apex_robotics_studio_ops
    EXPORT_NAME Ops
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_ops
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_ops
    PUBLIC
        Apex::Core
        Apex::Workcell
        Apex::IO
        Apex::Simulation
        Apex::Extension
        Apex::Platform)
ars_apply_common_options(ars_ops)

add_library(ars_catalog
    src/Catalog/ProjectCatalog.cpp)
add_library(Apex::Catalog ALIAS ars_catalog)
set_target_properties(ars_catalog PROPERTIES
    OUTPUT_NAME apex_robotics_studio_catalog
    EXPORT_NAME Catalog
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_catalog
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_catalog
    PUBLIC
        Apex::Core
        Apex::IO
        Apex::Job
        Apex::Workcell)
ars_apply_common_options(ars_catalog)

add_library(ars_session
    src/Session/ExecutionSession.cpp)
add_library(Apex::Session ALIAS ars_session)
set_target_properties(ars_session PROPERTIES
    OUTPUT_NAME apex_robotics_studio_session
    EXPORT_NAME Session
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_session
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ars_apply_common_options(ars_session)

add_library(ars_dashboard
    src/Dashboard/WorkbenchDashboard.cpp)
add_library(Apex::Dashboard ALIAS ars_dashboard)
set_target_properties(ars_dashboard PROPERTIES
    OUTPUT_NAME apex_robotics_studio_dashboard
    EXPORT_NAME Dashboard
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_dashboard
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_dashboard
    PUBLIC
        Apex::Catalog
        Apex::Extension
        Apex::IO
        Apex::Ops
        Apex::Session
        Apex::Simulation)
ars_apply_common_options(ars_dashboard)

add_library(ars_edit
    src/Edit/ProjectPatch.cpp)
add_library(Apex::Edit ALIAS ars_edit)
set_target_properties(ars_edit PROPERTIES
    OUTPUT_NAME apex_robotics_studio_edit
    EXPORT_NAME Edit
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_edit
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_edit
    PUBLIC
        Apex::IO
        Apex::Workcell)
ars_apply_common_options(ars_edit)

add_library(ars_recipe
    src/Recipe/ProcessRecipe.cpp)
add_library(Apex::Recipe ALIAS ars_recipe)
set_target_properties(ars_recipe PROPERTIES
    OUTPUT_NAME apex_robotics_studio_recipe
    EXPORT_NAME Recipe
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_recipe
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_recipe
    PUBLIC
        Apex::IO
        Apex::Job)
ars_apply_common_options(ars_recipe)

add_library(ars_quality
    src/Quality/ReleaseGate.cpp)
add_library(Apex::Quality ALIAS ars_quality)
set_target_properties(ars_quality PROPERTIES
    OUTPUT_NAME apex_robotics_studio_quality
    EXPORT_NAME Quality
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_quality
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_quality
    PUBLIC
        Apex::Analysis
        Apex::Extension
        Apex::IO
        Apex::Simulation)
ars_apply_common_options(ars_quality)

add_library(ars_diff
    src/Diff/ProjectDiff.cpp)
add_library(Apex::Diff ALIAS ars_diff)
set_target_properties(ars_diff PROPERTIES
    OUTPUT_NAME apex_robotics_studio_diff
    EXPORT_NAME Diff
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_diff
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_diff
    PUBLIC
        Apex::IO)
ars_apply_common_options(ars_diff)

add_library(ars_template
    src/Template/ProjectTemplate.cpp)
add_library(Apex::Template ALIAS ars_template)
set_target_properties(ars_template PROPERTIES
    OUTPUT_NAME apex_robotics_studio_template
    EXPORT_NAME Template
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_template
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_template
    PUBLIC
        Apex::IO)
ars_apply_common_options(ars_template)

add_library(ars_regression
    src/Regression/ProjectRegression.cpp)
add_library(Apex::Regression ALIAS ars_regression)
set_target_properties(ars_regression PROPERTIES
    OUTPUT_NAME apex_robotics_studio_regression
    EXPORT_NAME Regression
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_regression
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_regression
    PUBLIC
        Apex::IO
        Apex::Quality
        Apex::Simulation)
ars_apply_common_options(ars_regression)

add_library(ars_delivery
    src/Delivery/DeliveryDossier.cpp)
add_library(Apex::Delivery ALIAS ars_delivery)
set_target_properties(ars_delivery PROPERTIES
    OUTPUT_NAME apex_robotics_studio_delivery
    EXPORT_NAME Delivery
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_delivery
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_delivery
    PUBLIC
        Apex::Quality
        Apex::Session)
ars_apply_common_options(ars_delivery)

add_library(ars_governance
    src/Governance/ProjectApproval.cpp)
add_library(Apex::Governance ALIAS ars_governance)
set_target_properties(ars_governance PROPERTIES
    OUTPUT_NAME apex_robotics_studio_governance
    EXPORT_NAME Governance
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_governance
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_governance
    PUBLIC
        Apex::IO)
ars_apply_common_options(ars_governance)

add_library(ars_revision
    src/Revision/ProjectSnapshot.cpp)
add_library(Apex::Revision ALIAS ars_revision)
set_target_properties(ars_revision PROPERTIES
    OUTPUT_NAME apex_robotics_studio_revision
    EXPORT_NAME Revision
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_revision
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ars_apply_common_options(ars_revision)

add_library(ars_integration
    src/Integration/IntegrationExport.cpp
    src/Integration/Ros2WorkspaceExport.cpp)
add_library(Apex::Integration ALIAS ars_integration)
set_target_properties(ars_integration PROPERTIES
    OUTPUT_NAME apex_robotics_studio_integration
    EXPORT_NAME Integration
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_integration
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_integration
    PUBLIC
        Apex::Governance
        Apex::Quality
        Apex::Simulation
        Apex::IO)
ars_apply_common_options(ars_integration)

add_library(ars_trace
    src/Trace/AuditTrail.cpp)
add_library(Apex::Trace ALIAS ars_trace)
set_target_properties(ars_trace PROPERTIES
    OUTPUT_NAME apex_robotics_studio_trace
    EXPORT_NAME Trace
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_trace
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ars_apply_common_options(ars_trace)

add_library(ars_workflow
    src/Workflow/AutomationFlow.cpp)
add_library(Apex::Workflow ALIAS ars_workflow)
set_target_properties(ars_workflow PROPERTIES
    OUTPUT_NAME apex_robotics_studio_workflow
    EXPORT_NAME Workflow
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_workflow
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ars_apply_common_options(ars_workflow)

add_library(ars_sweep
    src/Sweep/ParameterSweep.cpp)
add_library(Apex::Sweep ALIAS ars_sweep)
set_target_properties(ars_sweep PROPERTIES
    OUTPUT_NAME apex_robotics_studio_sweep
    EXPORT_NAME Sweep
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_sweep
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
ars_apply_common_options(ars_sweep)

add_library(ars_visualization
    src/Visualization/SvgExporter.cpp)
add_library(Apex::Visualization ALIAS ars_visualization)
set_target_properties(ars_visualization PROPERTIES
    OUTPUT_NAME apex_robotics_studio_visualization
    EXPORT_NAME Visualization
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_visualization
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_visualization
    PUBLIC
        Apex::Core
        Apex::Workcell
        Apex::Planning
        Apex::IO
        Apex::Simulation)
ars_apply_common_options(ars_visualization)

add_library(ars_report
    src/Report/ProjectReportGenerator.cpp)
add_library(Apex::Report ALIAS ars_report)
set_target_properties(ars_report PROPERTIES
    OUTPUT_NAME apex_robotics_studio_report
    EXPORT_NAME Report
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(ars_report
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(ars_report
    PUBLIC
        Apex::Core
        Apex::IO
        Apex::Planning
        Apex::Workcell
        Apex::Analysis
        Apex::Simulation
        Apex::Extension)
ars_apply_common_options(ars_report)

if(ARS_BUILD_APP)
    add_executable(apex_robotics_studio
        src/App/main.cpp)

    target_link_libraries(apex_robotics_studio
        PRIVATE
            Apex::Core
            Apex::Workcell
            Apex::Planning
            Apex::Job
            Apex::IO
            Apex::Importer
            Apex::Analysis
            Apex::Simulation
            Apex::Extension
            Apex::Platform
            Apex::Bundle
            Apex::Ops
            Apex::Catalog
            Apex::Session
            Apex::Dashboard
            Apex::Edit
            Apex::Recipe
            Apex::Quality
            Apex::Diff
            Apex::Template
            Apex::Regression
            Apex::Delivery
            Apex::Governance
            Apex::Revision
            Apex::Integration
            Apex::Trace
            Apex::Workflow
            Apex::Sweep
            Apex::Visualization
            Apex::Report)
    ars_apply_common_options(apex_robotics_studio)
endif()

add_executable(apex_audit_tool_sample
    src/Tools/SampleAuditToolMain.cpp)
target_link_libraries(apex_audit_tool_sample
    PRIVATE
        Apex::IO)
ars_apply_common_options(apex_audit_tool_sample)


if(ARS_ENABLE_QT_FRONTEND)
    find_package(Qt6 COMPONENTS Widgets QUIET)
    if(Qt6Widgets_FOUND)
        set(ARS_QT_WIDGETS_TARGET Qt6::Widgets)
    else()
        find_package(Qt5 COMPONENTS Widgets REQUIRED)
        set(ARS_QT_WIDGETS_TARGET Qt5::Widgets)
    endif()

    add_executable(apex_robotics_workstation_qt
        src/WorkbenchQt/main.cpp
        src/WorkbenchQt/MainWindow.cpp
        src/WorkbenchQt/MainWindow.h)
    target_link_libraries(apex_robotics_workstation_qt
        PRIVATE
            ${ARS_QT_WIDGETS_TARGET}
            Apex::Catalog
            Apex::Dashboard
            Apex::IO
            Apex::Simulation
            Apex::Session
            Apex::Visualization)
    set_target_properties(apex_robotics_workstation_qt PROPERTIES AUTOMOC ON)
    ars_apply_common_options(apex_robotics_workstation_qt)

    install(TARGETS apex_robotics_workstation_qt
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(ARS_ENABLE_ROS2_BRIDGE)
    add_library(ars_ros2_bridge INTERFACE)
    target_compile_definitions(ars_ros2_bridge INTERFACE ARS_HAS_ROS2_BRIDGE=1)
endif()

if(ARS_ENABLE_TESTS)
    enable_testing()
    add_executable(apex_robotics_studio_tests
        tests/TestMain.cpp
        tests/TestSerialRobotModel.cpp
        tests/TestCollisionWorld.cpp
        tests/TestJointTrajectoryPlanner.cpp
        tests/TestStudioProjectSerializer.cpp
        tests/TestUrdfImporter.cpp
        tests/TestTrajectoryAnalyzer.cpp
        tests/TestSvgExporter.cpp
        tests/TestJobSimulationEngine.cpp
        tests/TestProjectAuditPlugin.cpp
        tests/TestRuntimeConfig.cpp
        tests/TestProjectBundle.cpp
        tests/TestBatchRunner.cpp
        tests/TestProjectCatalog.cpp
        tests/TestExecutionSession.cpp
        tests/TestWorkbenchDashboard.cpp
        tests/TestProjectPatch.cpp
        tests/TestProcessRecipe.cpp
        tests/TestReleaseGate.cpp
        tests/TestProjectDiff.cpp
        tests/TestProjectTemplate.cpp
        tests/TestProjectRegression.cpp
        tests/TestDeliveryDossier.cpp
        tests/TestProjectApproval.cpp
        tests/TestProjectSnapshot.cpp
        tests/TestIntegrationExport.cpp
        tests/TestAuditTrail.cpp
        tests/TestAutomationFlow.cpp
        tests/TestParameterSweep.cpp
        tests/TestRobotDescriptionImport.cpp
        tests/TestRobotDescriptionImportAdvanced.cpp
    tests/TestRobotDescriptionInspector.cpp
        tests/TestRos2WorkspaceExport.cpp)

    target_link_libraries(apex_robotics_studio_tests
        PRIVATE
            Apex::Core
            Apex::Workcell
            Apex::Planning
            Apex::Job
            Apex::IO
            Apex::Importer
            Apex::Analysis
            Apex::Simulation
            Apex::Extension
            Apex::Platform
            Apex::Bundle
            Apex::Ops
            Apex::Catalog
            Apex::Session
            Apex::Dashboard
            Apex::Edit
            Apex::Recipe
            Apex::Quality
            Apex::Diff
            Apex::Template
            Apex::Regression
            Apex::Delivery
            Apex::Governance
            Apex::Revision
            Apex::Integration
            Apex::Trace
            Apex::Workflow
            Apex::Sweep
            Apex::Visualization
            Apex::Report)
    ars_apply_common_options(apex_robotics_studio_tests)

    add_test(NAME apex_robotics_studio_tests COMMAND apex_robotics_studio_tests)
endif()

install(TARGETS ars_core ars_workcell ars_planning ars_job ars_io ars_importer ars_analysis ars_simulation ars_extension ars_platform ars_bundle ars_ops ars_catalog ars_session ars_dashboard ars_edit ars_recipe ars_quality ars_diff ars_template ars_regression ars_delivery ars_governance ars_revision ars_integration ars_trace ars_workflow ars_sweep ars_visualization ars_report
    EXPORT ApexRoboticsStudioTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(ARS_BUILD_APP)
    install(TARGETS apex_robotics_studio
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS apex_audit_tool_sample
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY samples/projects/ DESTINATION ${ARS_INSTALL_SAMPLESDIR})
install(DIRECTORY samples/urdf/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/urdf)
install(DIRECTORY samples/plugins/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/plugins)
install(DIRECTORY samples/batches/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/batches)
install(DIRECTORY samples/workstation/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/workstation)
install(DIRECTORY samples/recipes/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/recipes)
install(DIRECTORY samples/patches/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/patches)
install(DIRECTORY samples/templates/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/templates)
install(DIRECTORY samples/regression/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/regression)
install(DIRECTORY samples/delivery/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/delivery)
install(DIRECTORY samples/governance/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/governance)
install(DIRECTORY samples/snapshots/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/snapshots)
install(DIRECTORY samples/integration/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/integration)
install(DIRECTORY samples/flows/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/flows)
install(DIRECTORY samples/sweeps/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/sweeps)
install(DIRECTORY samples/trace/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/trace)
install(DIRECTORY samples/xacro/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/xacro)
install(DIRECTORY samples/ros2/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/ros2)
install(DIRECTORY samples/robot_description_packages/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/robot_description_packages)
install(DIRECTORY config/ DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/config)
install(FILES
    README.md
    docs/Architecture.md
    docs/InterviewGuide.md
    docs/CrossCompilationGuide.md
    docs/ReleaseEngineering.md
    docs/ProjectFileFormat.md
    docs/AdvancedCapabilities.md
    docs/UrdfImportGuide.md
    docs/JobProgrammingGuide.md
    docs/PluginArchitecture.md
    docs/RuntimeOperationsGuide.md
    docs/DeploymentAndExtensibility.md
    docs/BatchOperationsGuide.md
    docs/WorkstationGuide.md
    docs/QtWorkbenchGuide.md
    docs/RecipeAndPatchGuide.md
    docs/ReleaseGateGuide.md
    docs/ProjectDiffGuide.md
    docs/TemplateGuide.md
    docs/RegressionGuide.md
    docs/DeliveryDossierGuide.md
    docs/EnterpriseEnhancements_v09.md
    docs/ApprovalWorkflowGuide.md
    docs/SnapshotAndRollbackGuide.md
    docs/IntegrationExportGuide.md
    docs/EnterpriseEnhancements_v10.md
    docs/AuditTrailGuide.md
    docs/AutomationFlowGuide.md
    docs/ParameterSweepGuide.md
    docs/EnterpriseEnhancements_v11.md
    docs/RobotDescriptionAndRos2Guide.md
    docs/QtWorkbenchGuide_v12.md
    docs/EnterpriseEnhancements_v12.md
    docs/LocalValidationOutput_v12.txt
    docs/RobotDescriptionAndRos2Guide_v13.md
    docs/QtWorkbenchGuide_v13.md
    docs/EnterpriseEnhancements_v13.md
    docs/LocalValidationOutput_v13.txt
    DESTINATION ${CMAKE_INSTALL_DOCDIR})

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ApexRoboticsStudioConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ApexRoboticsStudioConfig.cmake
    INSTALL_DESTINATION ${ARS_INSTALL_CMAKEDIR})

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ApexRoboticsStudioConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(EXPORT ApexRoboticsStudioTargets
    NAMESPACE Apex::
    FILE ApexRoboticsStudioTargets.cmake
    DESTINATION ${ARS_INSTALL_CMAKEDIR})

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ApexRoboticsStudioConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ApexRoboticsStudioConfigVersion.cmake
    DESTINATION ${ARS_INSTALL_CMAKEDIR})

set(CPACK_PACKAGE_NAME "ApexRoboticsStudio")
set(CPACK_PACKAGE_VENDOR "OpenAI Demo")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP;TGZ")
include(CPack)
