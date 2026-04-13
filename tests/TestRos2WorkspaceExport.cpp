#include "TestFramework.h"
#include "apex/integration/Ros2WorkspaceExport.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

ARS_TEST(TestRos2WorkspaceExportGeneratesDescriptionBringupAndMoveItSkeleton)
{
    apex::io::StudioProject project;
    project.SchemaVersion = 5;
    project.ProjectName = "Export Demo";
    project.RobotName = "ExportBot";
    project.DescriptionSource.SourceKind = "xacro";
    project.Joints.push_back({"joint_1", 0.40, -1.0, 1.0});
    project.Joints.push_back({"joint_2", 0.30, -1.0, 1.0});
    project.Obstacles.push_back({"fixture", {{0.1, 0.2, 0.0}, {0.2, 0.3, 0.4}}});

    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "apex_ros2_export_v13";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot);

    const std::filesystem::path meshSource = tempRoot / "source_meshes" / "base.stl";
    std::filesystem::create_directories(meshSource.parent_path());
    {
        std::ofstream meshStream(meshSource);
        meshStream << "solid base\nendsolid base\n";
    }

    const std::filesystem::path expandedUrdf = tempRoot / "exportbot.expanded.urdf";
    {
        std::ofstream urdfStream(expandedUrdf);
        urdfStream << R"(<robot name="ExportBot">
  <link name="base_link">
    <visual><geometry><mesh filename="package://legacy_pkg/meshes/base.stl" scale="1 1 1"/></geometry></visual>
  </link>
  <joint name="joint_1" type="revolute">
    <origin xyz="0.40 0 0" rpy="0 0 0"/>
    <limit lower="-1.0" upper="1.0" effort="10" velocity="1"/>
  </joint>
  <joint name="joint_2" type="revolute">
    <origin xyz="0.30 0 0" rpy="0 0 0"/>
    <limit lower="-1.0" upper="1.0" effort="10" velocity="1"/>
  </joint>
</robot>)";
    }
    project.DescriptionSource.SourcePath = expandedUrdf.string();
    project.DescriptionSource.ExpandedDescriptionPath = expandedUrdf.string();
    project.MeshResources.push_back({"base_link", "visual_mesh", "package://legacy_pkg/meshes/base.stl", meshSource.string(), "legacy_pkg", {1.0, 1.0, 1.0}});
    project.DescriptionSource.PackageDependencies = {"legacy_pkg", "vendor_support_pkg"};

    const std::filesystem::path outputRoot = tempRoot / "generated";
    const apex::integration::Ros2WorkspaceExporter exporter;
    const auto result = exporter.ExportWorkspace(project, outputRoot);

    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "package.xml"), "Description package.xml should exist");
    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "urdf/exportbot.urdf"), "Description URDF should exist");
    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "meshes/imported/base.stl"), "Resolved mesh should be copied into description package");
    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "launch/display.launch.py"), "Description launch file should exist");
    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "launch/controllers.launch.py"), "Description controllers launch should exist");
    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "config/initial_positions.yaml"), "Initial positions config should exist in description package");
    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "config/package_dependencies.yaml"), "Description package dependency catalog should exist");
    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "config/resource_manifest.json"), "Description resource manifest should exist");
    apex::tests::Require(std::filesystem::exists(result.DescriptionPackageRoot / "source/exportbot.expanded.urdf"), "Original robot description source should be copied into the description package");
    apex::tests::Require(std::filesystem::exists(result.BringupPackageRoot / "package.xml"), "Bringup package.xml should exist");
    apex::tests::Require(std::filesystem::exists(result.BringupPackageRoot / "launch/bringup.launch.py"), "Bringup launch should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "package.xml"), "MoveIt package.xml should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "config/moveit_controllers.yaml"), "MoveIt controllers config should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "config/ros2_controllers.yaml"), "ros2_control skeleton config should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "config/scene_objects.yaml"), "Scene objects config should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "config/moveit_cpp.yaml"), "MoveItCpp config should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "config/planning_scene_monitor_parameters.yaml"), "Planning scene monitor parameters should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "config/initial_positions.yaml"), "MoveIt initial positions should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "src/apex_scene_loader_node.cpp"), "Scene loader source should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "rviz/moveit_demo.rviz"), "MoveIt RViz config should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "launch/move_group.launch.py"), "Move-group launch should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "launch/bringup_with_moveit.launch.py"), "Bringup+MoveIt launch should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItPackageRoot / "launch/full_stack_demo.launch.py"), "Full-stack demo launch should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItTaskPackageRoot / "package.xml"), "MoveIt task demo package.xml should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItTaskPackageRoot / "src/apex_mtc_demo.cpp"), "MoveIt task demo source should exist");
    apex::tests::Require(std::filesystem::exists(result.MoveItTaskPackageRoot / "launch/mtc_demo.launch.py"), "MoveIt task demo launch should exist");
    apex::tests::Require(std::filesystem::exists(result.WorkspaceRoot / "workspace_manifest.json"), "Workspace manifest should exist");
    apex::tests::Require(std::filesystem::exists(result.WorkspaceRoot / "dependency_catalog.md"), "Workspace dependency catalog should exist");
    apex::tests::Require(std::filesystem::exists(result.WorkspaceRoot / "workspace_doctor.py"), "Workspace doctor script should exist");

    const auto readFile = [](const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };

    const std::string descriptionPackage = readFile(result.DescriptionPackageRoot / "package.xml");
    const std::string exportedUrdf = readFile(result.DescriptionPackageRoot / "urdf/exportbot.urdf");
    const std::string workspaceReadme = readFile(result.WorkspaceRoot / "README.md");
    const std::string workspaceMeta = readFile(result.WorkspaceRoot / "colcon.meta");
    const std::string controllersLaunch = readFile(result.DescriptionPackageRoot / "launch/controllers.launch.py");
    const std::string initialPositions = readFile(result.DescriptionPackageRoot / "config/initial_positions.yaml");
    const std::string packageDependencies = readFile(result.DescriptionPackageRoot / "config/package_dependencies.yaml");
    const std::string resourceManifest = readFile(result.DescriptionPackageRoot / "config/resource_manifest.json");
    const std::string moveitSource = readFile(result.MoveItPackageRoot / "src/apex_moveit_smoke_test.cpp");
    const std::string sceneLoaderSource = readFile(result.MoveItPackageRoot / "src/apex_scene_loader_node.cpp");
    const std::string bringupLaunch = readFile(result.BringupPackageRoot / "launch/bringup.launch.py");
    const std::string moveitLaunch = readFile(result.MoveItPackageRoot / "launch/demo_moveit.launch.py");
    const std::string moveGroupLaunch = readFile(result.MoveItPackageRoot / "launch/move_group.launch.py");
    const std::string combinedLaunch = readFile(result.MoveItPackageRoot / "launch/bringup_with_moveit.launch.py");
    const std::string fullStackLaunch = readFile(result.MoveItPackageRoot / "launch/full_stack_demo.launch.py");
    const std::string moveitCppYaml = readFile(result.MoveItPackageRoot / "config/moveit_cpp.yaml");
    const std::string sceneYaml = readFile(result.MoveItPackageRoot / "config/scene_objects.yaml");
    const std::string mtcSource = readFile(result.MoveItTaskPackageRoot / "src/apex_mtc_demo.cpp");
    const std::string mtcLaunch = readFile(result.MoveItTaskPackageRoot / "launch/mtc_demo.launch.py");
    const std::string workspaceManifest = readFile(result.WorkspaceRoot / "workspace_manifest.json");
    const std::string dependencyCatalog = readFile(result.WorkspaceRoot / "dependency_catalog.md");
    const std::string workspaceDoctor = readFile(result.WorkspaceRoot / "workspace_doctor.py");
    apex::tests::Require(descriptionPackage.find("robot_state_publisher") != std::string::npos, "Description package should reference robot_state_publisher");
    apex::tests::Require(workspaceReadme.find("Suggested next steps") != std::string::npos, "Workspace README should include integration guidance");
    apex::tests::Require(workspaceMeta.find("export_demo_moveit_config") != std::string::npos, "colcon.meta should mention the MoveIt package");
    apex::tests::Require(controllersLaunch.find("ros2_control_node") != std::string::npos, "Description controllers launch should start ros2_control_node");
    apex::tests::Require(initialPositions.find("joint_1") != std::string::npos, "Initial positions YAML should list exported joints");
    apex::tests::Require(exportedUrdf.find("package://export_demo_description/meshes/imported/base.stl") != std::string::npos, "Exported URDF should rewrite mesh URI to generated description package");
    apex::tests::Require(moveitSource.find("MoveGroupInterface") != std::string::npos, "MoveIt skeleton should reference MoveGroupInterface");
    apex::tests::Require(moveitSource.find("PlanningSceneInterface") != std::string::npos, "MoveIt skeleton should reference PlanningSceneInterface");
    apex::tests::Require(sceneLoaderSource.find("applyCollisionObjects") != std::string::npos, "Scene loader should populate planning scene obstacles");
    apex::tests::Require(sceneYaml.find("fixture") != std::string::npos, "Scene objects config should list exported obstacles");
    apex::tests::Require(packageDependencies.find("legacy_pkg") != std::string::npos, "Description package dependency catalog should include mesh package dependency");
    apex::tests::Require(packageDependencies.find("vendor_support_pkg") != std::string::npos, "Description package dependency catalog should preserve imported package list");
    apex::tests::Require(resourceManifest.find("mesh_resources") != std::string::npos, "Description resource manifest should list mesh resources");
    apex::tests::Require(moveitLaunch.find("apex_scene_loader_node") != std::string::npos, "MoveIt demo launch should start the scene loader node");
    apex::tests::Require(moveGroupLaunch.find("scene_yaml") != std::string::npos, "Move-group launch should pass the scene YAML to the loader node");
    apex::tests::Require(combinedLaunch.find("bringup.launch.py") != std::string::npos, "Combined launch should include bringup.launch.py");
    apex::tests::Require(fullStackLaunch.find("mtc_demo.launch.py") != std::string::npos, "Full-stack launch should include the MoveIt task demo when enabled");
    apex::tests::Require(moveitCppYaml.find("planning_group") != std::string::npos, "MoveItCpp YAML should declare the planning group");
    apex::tests::Require(moveitLaunch.find("rviz2") != std::string::npos, "MoveIt demo launch should include RViz");
    apex::tests::Require(bringupLaunch.find("display.launch.py") != std::string::npos, "Bringup launch should include description display launch");
    apex::tests::Require(mtcSource.find("MoveGroupInterface") != std::string::npos, "MoveIt task demo should reference MoveGroupInterface");
    apex::tests::Require(mtcLaunch.find("apex_mtc_demo") != std::string::npos, "MoveIt task launch should start task demo node");
    apex::tests::Require(workspaceManifest.find("vendor_support_pkg") != std::string::npos, "Workspace manifest should record description package dependencies");
    apex::tests::Require(dependencyCatalog.find("legacy_pkg") != std::string::npos, "Dependency catalog should include mesh package dependencies");
    apex::tests::Require(workspaceDoctor.find("Workspace Doctor") != std::string::npos, "Workspace doctor script should include a readable banner");
    apex::tests::Require(result.DescriptionPackageName == "export_demo_description", "Description package name should be reported in export result");
    apex::tests::Require(result.MoveItTaskPackageName == "export_demo_moveit_task_demo", "MoveIt task package name should be reported in export result");
}
