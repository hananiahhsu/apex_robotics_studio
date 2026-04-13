#include "apex/integration/Ros2WorkspaceExport.h"

#include "apex/core/MathTypes.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace apex::integration
{
    namespace
    {
        std::string SanitizeToken(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
            {
                if (std::isalnum(ch) != 0)
                {
                    return static_cast<char>(std::tolower(ch));
                }
                return '_';
            });
            while (!value.empty() && value.front() == '_')
            {
                value.erase(value.begin());
            }
            while (!value.empty() && value.back() == '_')
            {
                value.pop_back();
            }
            if (value.empty())
            {
                value = "apex_project";
            }
            return value;
        }

        void WriteTextFile(const std::filesystem::path& filePath, const std::string& text)
        {
            std::filesystem::create_directories(filePath.parent_path());
            std::ofstream stream(filePath);
            if (!stream)
            {
                throw std::runtime_error("Failed to write file: " + filePath.string());
            }
            stream << text;
        }

        std::string ReadTextFile(const std::filesystem::path& filePath)
        {
            std::ifstream stream(filePath);
            if (!stream)
            {
                return {};
            }
            return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        }

        std::string EscapeYaml(const std::string& value)
        {
            std::string output = value;
            std::replace(output.begin(), output.end(), '\n', ' ');
            return output;
        }

        std::string BuildJointListYaml(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            for (const auto& joint : project.Joints)
            {
                builder << "    - " << joint.Name << "\n";
            }
            return builder.str();
        }

        std::string BuildProjectConfigYaml(const apex::io::StudioProject& project, const std::string& descriptionPackage)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(6);
            builder << "apex_project:\n"
                    << "  ros__parameters:\n"
                    << "    project_name: \"" << EscapeYaml(project.ProjectName) << "\"\n"
                    << "    robot_name: \"" << EscapeYaml(project.RobotName) << "\"\n"
                    << "    description_package: \"" << descriptionPackage << "\"\n"
                    << "    planning_group: \"manipulator\"\n"
                    << "    joint_names:\n"
                    << BuildJointListYaml(project)
                    << "    obstacles:\n";
            for (const auto& obstacle : project.Obstacles)
            {
                builder << "      - name: \"" << EscapeYaml(obstacle.Name) << "\"\n"
                        << "        min: [" << obstacle.Bounds.Min.X << ", " << obstacle.Bounds.Min.Y << ", " << obstacle.Bounds.Min.Z << "]\n"
                        << "        max: [" << obstacle.Bounds.Max.X << ", " << obstacle.Bounds.Max.Y << ", " << obstacle.Bounds.Max.Z << "]\n";
            }
            return builder.str();
        }

        std::string BuildSceneObjectsYaml(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(6);
            builder << "scene_objects:\n";
            for (const auto& obstacle : project.Obstacles)
            {
                const double sizeX = obstacle.Bounds.Max.X - obstacle.Bounds.Min.X;
                const double sizeY = obstacle.Bounds.Max.Y - obstacle.Bounds.Min.Y;
                const double sizeZ = obstacle.Bounds.Max.Z - obstacle.Bounds.Min.Z;
                const double centerX = (obstacle.Bounds.Min.X + obstacle.Bounds.Max.X) * 0.5;
                const double centerY = (obstacle.Bounds.Min.Y + obstacle.Bounds.Max.Y) * 0.5;
                const double centerZ = (obstacle.Bounds.Min.Z + obstacle.Bounds.Max.Z) * 0.5;
                builder << "  - id: \"" << EscapeYaml(obstacle.Name) << "\"\n"
                        << "    primitive: box\n"
                        << "    size: [" << sizeX << ", " << sizeY << ", " << sizeZ << "]\n"
                        << "    pose: [" << centerX << ", " << centerY << ", " << centerZ << ", 0.0, 0.0, 0.0, 1.0]\n";
            }
            return builder.str();
        }

        std::string BuildPackageDependenciesYaml(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder << "robot_description_dependencies:\n";
            for (const auto& dependency : project.DescriptionSource.PackageDependencies)
            {
                builder << "  - " << dependency << "\n";
            }
            return builder.str();
        }

        std::string BuildWorkspaceManifestJson(
            const apex::io::StudioProject& project,
            const std::string& descriptionPackage,
            const std::string& bringupPackage,
            const std::string& moveitPackage,
            const std::string& mtcPackage,
            bool includeMoveIt,
            bool includeMtc)
        {
            std::ostringstream builder;
            builder << "{\n"
                    << "  \"project_name\": \"" << EscapeYaml(project.ProjectName) << "\",\n"
                    << "  \"robot_name\": \"" << EscapeYaml(project.RobotName) << "\",\n"
                    << "  \"packages\": [\n"
                    << "    \"" << descriptionPackage << "\",\n"
                    << "    \"" << bringupPackage << "\"";
            if (includeMoveIt)
            {
                builder << ",\n    \"" << moveitPackage << "\"";
            }
            if (includeMtc)
            {
                builder << ",\n    \"" << mtcPackage << "\"";
            }
            builder << "\n  ],\n"
                    << "  \"description_package_dependencies\": [";
            for (std::size_t index = 0; index < project.DescriptionSource.PackageDependencies.size(); ++index)
            {
                if (index > 0)
                {
                    builder << ", ";
                }
                builder << "\"" << EscapeYaml(project.DescriptionSource.PackageDependencies[index]) << "\"";
            }
            builder << "]\n"
                    << "}\n";
            return builder.str();
        }

        std::string BuildResourceManifestJson(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(6);
            builder << "{\n"
                    << "  \"project_name\": \"" << EscapeYaml(project.ProjectName) << "\",\n"
                    << "  \"robot_name\": \"" << EscapeYaml(project.RobotName) << "\",\n"
                    << "  \"source_kind\": \"" << EscapeYaml(project.DescriptionSource.SourceKind) << "\",\n"
                    << "  \"mesh_resources\": [\n";
            for (std::size_t index = 0; index < project.MeshResources.size(); ++index)
            {
                const auto& mesh = project.MeshResources[index];
                builder << "    {\"link_name\": \"" << EscapeYaml(mesh.LinkName)
                        << "\", \"role\": \"" << EscapeYaml(mesh.Role)
                        << "\", \"uri\": \"" << EscapeYaml(mesh.Uri)
                        << "\", \"package_name\": \"" << EscapeYaml(mesh.PackageName)
                        << "\", \"resolved_path\": \"" << EscapeYaml(mesh.ResolvedPath)
                        << "\", \"scale\": [" << mesh.Scale.X << ", " << mesh.Scale.Y << ", " << mesh.Scale.Z << "]}";
                if (index + 1 < project.MeshResources.size())
                {
                    builder << ',';
                }
                builder << "\n";
            }
            builder << "  ]\n}\n";
            return builder.str();
        }

        std::string BuildDependencyCatalogMarkdown(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder << "# Dependency Catalog\n\n"
                    << "## Description Packages\n\n";
            if (project.DescriptionSource.PackageDependencies.empty())
            {
                builder << "- None recorded\n";
            }
            else
            {
                for (const auto& dependency : project.DescriptionSource.PackageDependencies)
                {
                    builder << "- `" << dependency << "`\n";
                }
            }
            builder << "\n## Mesh Resources\n\n"
                    << "| Link | Role | URI | Package | Resolved Path |\n"
                    << "|---|---|---|---|---|\n";
            if (project.MeshResources.empty())
            {
                builder << "| None | - | - | - | - |\n";
            }
            else
            {
                for (const auto& mesh : project.MeshResources)
                {
                    builder << "| " << mesh.LinkName << " | " << mesh.Role << " | `" << mesh.Uri << "` | "
                            << (mesh.PackageName.empty() ? "-" : mesh.PackageName) << " | `"
                            << (mesh.ResolvedPath.empty() ? std::string("<unresolved>") : mesh.ResolvedPath) << "` |\n";
                }
            }
            return builder.str();
        }

        std::string BuildWorkspaceDoctorPy(
            const std::string& descriptionPackage,
            const std::string& bringupPackage,
            const std::string& moveitPackage,
            const std::string& mtcPackage,
            bool includeMoveIt,
            bool includeMtc)
        {
            std::ostringstream builder;
            builder << "#!/usr/bin/env python3\n"
                    << "import json\n"
                    << "from pathlib import Path\n\n"
                    << "WORKSPACE_ROOT = Path(__file__).resolve().parent\n"
                    << "required = [\n"
                    << "    ('" << descriptionPackage << "', WORKSPACE_ROOT / 'src' / '" << descriptionPackage << "'),\n"
                    << "    ('" << bringupPackage << "', WORKSPACE_ROOT / 'src' / '" << bringupPackage << "'),\n";
            if (includeMoveIt)
            {
                builder << "    ('" << moveitPackage << "', WORKSPACE_ROOT / 'src' / '" << moveitPackage << "'),\n";
            }
            if (includeMtc)
            {
                builder << "    ('" << mtcPackage << "', WORKSPACE_ROOT / 'src' / '" << mtcPackage << "'),\n";
            }
            builder << "]\n\n"
                    << "missing = [name for name, path in required if not path.exists()]\n"
                    << "manifest = WORKSPACE_ROOT / 'workspace_manifest.json'\n"
                    << "print('Apex ROS2 Workspace Doctor')\n"
                    << "print('Workspace:', WORKSPACE_ROOT)\n"
                    << "print('Manifest :', manifest)\n"
                    << "if missing:\n"
                    << "    print('Missing packages:')\n"
                    << "    for item in missing:\n"
                    << "        print('  -', item)\n"
                    << "else:\n"
                    << "    print('All generated packages are present.')\n"
                    << "if manifest.exists():\n"
                    << "    print('Manifest summary:')\n"
                    << "    data = json.loads(manifest.read_text(encoding='utf-8'))\n"
                    << "    print(json.dumps(data, indent=2))\n"
                    << "else:\n"
                    << "    print('workspace_manifest.json is missing.')\n"
                    << "raise SystemExit(1 if missing else 0)\n";
            (void)moveitPackage;
            return builder.str();
        }

        std::string BuildFullStackDemoLaunchPy(
            const std::string& bringupPackage,
            const std::string& moveitPackage,
            const std::string& mtcPackage,
            bool includeMtc)
        {
            std::ostringstream builder;
            builder << "from launch import LaunchDescription\n"
                    << "from launch.actions import IncludeLaunchDescription\n"
                    << "from launch.launch_description_sources import PythonLaunchDescriptionSource\n"
                    << "from ament_index_python.packages import get_package_share_directory\n"
                    << "import os\n\n"
                    << "def generate_launch_description():\n"
                    << "    bringup_share = get_package_share_directory('" << bringupPackage << "')\n"
                    << "    moveit_share = get_package_share_directory('" << moveitPackage << "')\n"
                    << "    actions = [\n"
                    << "        IncludeLaunchDescription(PythonLaunchDescriptionSource(os.path.join(bringup_share, 'launch', 'bringup.launch.py'))),\n"
                    << "        IncludeLaunchDescription(PythonLaunchDescriptionSource(os.path.join(moveit_share, 'launch', 'bringup_with_moveit.launch.py')))\n"
                    << "    ]\n";
            if (includeMtc)
            {
                builder << "    mtc_share = get_package_share_directory('" << mtcPackage << "')\n"
                        << "    actions.append(IncludeLaunchDescription(PythonLaunchDescriptionSource(os.path.join(mtc_share, 'launch', 'mtc_demo.launch.py'))))\n";
            }
            builder << "    return LaunchDescription(actions)\n";
            return builder.str();
        }

        std::string BuildDescriptionPackageXml(const std::string& packageName)
        {
            std::ostringstream builder;
            builder << R"(<package format="3">
  <name>)" << packageName << R"(</name>
  <version>0.1.0</version>
  <description>Apex Robotics Studio generated robot description package.</description>
  <maintainer email="demo@example.com">Apex Robotics Studio</maintainer>
  <license>MIT</license>

  <buildtool_depend>ament_cmake</buildtool_depend>
  <exec_depend>robot_state_publisher</exec_depend>
  <exec_depend>joint_state_publisher_gui</exec_depend>
  <exec_depend>rviz2</exec_depend>
  <exec_depend>xacro</exec_depend>
  <exec_depend>ament_index_python</exec_depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
)";
            return builder.str();
        }

        std::string BuildDescriptionCMakeLists(const std::string& packageName)
        {
            std::ostringstream builder;
            builder << R"(cmake_minimum_required(VERSION 3.16)
project()" << packageName << R"(()

find_package(ament_cmake REQUIRED)

install(DIRECTORY urdf meshes launch rviz config source DESTINATION share/${PROJECT_NAME})

ament_package()
)";
            return builder.str();
        }

        std::string BuildDescriptionDisplayLaunchPy(const std::string& packageName, const std::string& urdfFileName)
        {
            std::ostringstream builder;
            builder << R"(from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    share = get_package_share_directory(')" << packageName << R"(')
    urdf_path = os.path.join(share, 'urdf', ')" << urdfFileName << R"(')
    rviz_path = os.path.join(share, 'rviz', 'view_robot.rviz')
    with open(urdf_path, 'r', encoding='utf-8') as stream:
        robot_description = stream.read()
    return LaunchDescription([
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            parameters=[{'robot_description': robot_description}],
            output='screen'),
        Node(
            package='joint_state_publisher_gui',
            executable='joint_state_publisher_gui',
            output='screen'),
        Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', rviz_path],
            output='screen')
    ])
)";
            return builder.str();
        }

        std::string BuildDefaultRviz()
        {
            return std::string(
                "Panels:\\n"
                "  - Class: rviz_common/Displays\\n"
                "Visualization Manager:\\n"
                "  Class: \\\"\\\"\\n"
                "  Displays:\\n"
                "    - Class: rviz_default_plugins/Grid\\n"
                "      Name: Grid\\n"
                "    - Class: rviz_default_plugins/RobotModel\\n"
                "      Name: Robot Model\\n"
                "      Robot Description: robot_description\\n"
                "    - Class: rviz_default_plugins/TF\\n"
                "      Name: TF\\n");
        }

        std::string BuildRos2ControllersYaml(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder << "controller_manager:\n"
                    << "  ros__parameters:\n"
                    << "    update_rate: 100\n"
                    << "    joint_state_broadcaster:\n"
                    << "      type: joint_state_broadcaster/JointStateBroadcaster\n"
                    << "    arm_controller:\n"
                    << "      type: joint_trajectory_controller/JointTrajectoryController\n\n"
                    << "arm_controller:\n"
                    << "  ros__parameters:\n"
                    << "    joints:\n";
            for (const auto& joint : project.Joints)
            {
                builder << "      - " << joint.Name << "\n";
            }
            builder << "    command_interfaces: [position]\n"
                    << "    state_interfaces: [position, velocity]\n";
            return builder.str();
        }

        std::string BuildInitialPositionsYaml(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(6);
            builder << "initial_positions:\n";
            for (const auto& joint : project.Joints)
            {
                builder << "  " << joint.Name << ": 0.0\n";
            }
            return builder.str();
        }

        std::string BuildMoveItCppYaml(const std::string& planningGroup)
        {
            std::ostringstream builder;
            builder << "planning_scene_monitor_options:\n"
                    << "  name: apex_planning_scene_monitor\n"
                    << "  robot_description: robot_description\n"
                    << "  joint_state_topic: /joint_states\n"
                    << "plan_request_params:\n"
                    << "  planning_pipeline: ompl\n"
                    << "  planning_attempts: 1\n"
                    << "  planning_time: 5.0\n"
                    << "moveit_cpp:\n"
                    << "  planning_group: " << planningGroup << "\n";
            return builder.str();
        }

        std::string BuildPlanningSceneMonitorParametersYaml()
        {
            return std::string(
                "planning_scene_monitor_parameters:\n"
                "  publish_planning_scene: true\n"
                "  publish_geometry_updates: true\n"
                "  publish_state_updates: true\n"
                "  publish_transforms_updates: true\n");
        }

        std::string BuildControllersLaunchPy(const std::string& descriptionPackage, const std::string& controllersFile)
        {
            std::ostringstream builder;
            builder
                << "from launch import LaunchDescription\n"
                << "from launch_ros.actions import Node\n"
                << "from ament_index_python.packages import get_package_share_directory\n"
                << "import os\n\n"
                << "def generate_launch_description():\n"
                << "    description_share = get_package_share_directory('" << descriptionPackage << "')\n"
                << "    controllers = os.path.join(description_share, 'config', '" << controllersFile << "')\n"
                << "    return LaunchDescription([\n"
                << "        Node(package='controller_manager', executable='ros2_control_node', parameters=[controllers], output='screen'),\n"
                << "        Node(package='controller_manager', executable='spawner', arguments=['joint_state_broadcaster'], output='screen'),\n"
                << "        Node(package='controller_manager', executable='spawner', arguments=['arm_controller'], output='screen')\n"
                << "    ])\n";
            return builder.str();
        }
        std::string BuildMoveGroupLaunchPy(const std::string& packageName, const std::string& descriptionPackage)
        {
            std::ostringstream builder;
            builder
                << "from launch import LaunchDescription\n"
                << "from launch.actions import IncludeLaunchDescription\n"
                << "from launch.launch_description_sources import PythonLaunchDescriptionSource\n"
                << "from launch_ros.actions import Node\n"
                << "from ament_index_python.packages import get_package_share_directory\n"
                << "import os\n\n"
                << "def generate_launch_description():\n"
                << "    moveit_share = get_package_share_directory('" << packageName << "')\n"
                << "    description_share = get_package_share_directory('" << descriptionPackage << "')\n"
                << "    display_launch = os.path.join(description_share, 'launch', 'display.launch.py')\n"
                << "    scene_yaml = os.path.join(moveit_share, 'config', 'scene_objects.yaml')\n"
                << "    return LaunchDescription([\n"
                << "        IncludeLaunchDescription(PythonLaunchDescriptionSource(display_launch)),\n"
                << "        Node(package='" << packageName << "', executable='apex_scene_loader_node', parameters=[{'scene_yaml': scene_yaml}], output='screen'),\n"
                << "        Node(package='" << packageName << "', executable='apex_moveit_smoke_test', output='screen')\n"
                << "    ])\n";
            return builder.str();
        }
        std::string BuildBringupWithMoveItLaunchPy(const std::string& bringupPackage, const std::string& moveitPackage)
        {
            std::ostringstream builder;
            builder
                << "from launch import LaunchDescription\n"
                << "from launch.actions import IncludeLaunchDescription\n"
                << "from launch.launch_description_sources import PythonLaunchDescriptionSource\n"
                << "from ament_index_python.packages import get_package_share_directory\n"
                << "import os\n\n"
                << "def generate_launch_description():\n"
                << "    bringup_launch = os.path.join(get_package_share_directory('" << bringupPackage << "'), 'launch', 'bringup.launch.py')\n"
                << "    moveit_launch = os.path.join(get_package_share_directory('" << moveitPackage << "'), 'launch', 'move_group.launch.py')\n"
                << "    return LaunchDescription([\n"
                << "        IncludeLaunchDescription(PythonLaunchDescriptionSource(bringup_launch)),\n"
                << "        IncludeLaunchDescription(PythonLaunchDescriptionSource(moveit_launch))\n"
                << "    ])\n";
            return builder.str();
        }
        std::string BuildWorkspaceReadme(
            const apex::io::StudioProject& project,
            const std::string& descriptionPackage,
            const std::string& bringupPackage,
            const std::string& moveitPackage,
            const std::string& mtcPackage,
            bool includeMoveIt,
            bool includeMtc)
        {
            std::ostringstream builder;
            builder << "# Generated Apex ROS 2 Workspace\n\n"
                    << "Project: " << project.ProjectName << "\n"
                    << "Robot: " << project.RobotName << "\n\n"
                    << "## Build\n\n"
                    << "```bash\n"
                    << "colcon build --symlink-install\n"
                    << "source install/setup.bash\n"
                    << "```\n\n"
                    << "## Packages\n\n"
                    << "- " << descriptionPackage << "\n"
                    << "- " << bringupPackage << "\n";
            if (includeMoveIt)
            {
                builder << "- " << moveitPackage << "\n";
            }
            if (includeMtc)
            {
                builder << "- " << mtcPackage << "\n";
            }
            if (!project.DescriptionSource.PackageDependencies.empty())
            {
                builder << "\n## Referenced robot description packages\n\n";
                for (const auto& dependency : project.DescriptionSource.PackageDependencies)
                {
                    builder << "- " << dependency << "\n";
                }
            }
            builder << "\n## Suggested next steps\n\n"
                    << "1. Replace generated robot description with your production URDF/Xacro if needed.\n"
                    << "2. Point controller parameters at your real ros2_control setup.\n"
                    << "3. Replace the smoke-test node with production motion goals and driver integration.\n";
            return builder.str();
        }

        std::string BuildWorkspaceGitIgnore()
        {
            return std::string("build/\ninstall/\nlog/\n.colcon/\n");
        }

        std::string BuildWorkspaceColconMeta(const std::string& descriptionPackage, const std::string& bringupPackage, const std::string& moveitPackage, const std::string& mtcPackage, bool includeMtc)
        {
            std::ostringstream builder;
            builder << "{\n"
                    << "  \"names\": {\n"
                    << "    \"" << descriptionPackage << "\": { \"cmake-args\": [\"-DCMAKE_BUILD_TYPE=Release\"] },\n"
                    << "    \"" << bringupPackage << "\": { \"cmake-args\": [\"-DCMAKE_BUILD_TYPE=Release\"] },\n"
                    << "    \"" << moveitPackage << "\": { \"cmake-args\": [\"-DCMAKE_BUILD_TYPE=Release\"] }";
            if (includeMtc)
            {
                builder << ",\n    \"" << mtcPackage << "\": { \"cmake-args\": [\"-DCMAKE_BUILD_TYPE=Release\"] }";
            }
            builder << "\n  }\n"
                    << "}\n";
            return builder.str();
        }

        std::string BuildGeneratedUrdf(
            const apex::io::StudioProject& project,
            const std::string&,
            const std::unordered_map<std::string, std::string>& meshUriMap)
        {
            std::string xml;
            if (!project.DescriptionSource.ExpandedDescriptionPath.empty())
            {
                xml = ReadTextFile(project.DescriptionSource.ExpandedDescriptionPath);
            }
            if (xml.empty() && !project.DescriptionSource.SourcePath.empty())
            {
                const std::filesystem::path sourcePath(project.DescriptionSource.SourcePath);
                if (sourcePath.extension() == ".urdf")
                {
                    xml = ReadTextFile(sourcePath);
                }
            }
            if (xml.empty())
            {
                std::ostringstream builder;
                builder << "<robot name=\"" << project.RobotName << "\">\n"
                        << "  <link name=\"base_link\"/>\n";
                std::string parent = "base_link";
                std::size_t index = 0;
                for (const auto& joint : project.Joints)
                {
                    const std::string linkName = "link_" + std::to_string(++index);
                    builder << "  <link name=\"" << linkName << "\"/>\n"
                            << "  <joint name=\"" << joint.Name << "\" type=\"revolute\">\n"
                            << "    <parent link=\"" << parent << "\"/>\n"
                            << "    <child link=\"" << linkName << "\"/>\n"
                            << "    <origin xyz=\"" << joint.LinkLength << " 0 0\" rpy=\"0 0 0\"/>\n"
                            << "    <axis xyz=\"0 0 1\"/>\n"
                            << "    <limit lower=\"" << joint.MinAngleRad << "\" upper=\"" << joint.MaxAngleRad << "\" effort=\"10\" velocity=\"1.5\"/>\n"
                            << "  </joint>\n";
                    parent = linkName;
                }
                builder << "</robot>\n";
                xml = builder.str();
            }

            for (const auto& [oldUri, newUri] : meshUriMap)
            {
                std::size_t pos = 0;
                while ((pos = xml.find(oldUri, pos)) != std::string::npos)
                {
                    xml.replace(pos, oldUri.size(), newUri);
                    pos += newUri.size();
                }
            }
            return xml;
        }

        void CopyMeshResources(
            const apex::io::StudioProject& project,
            const std::filesystem::path& descriptionPackageRoot,
            const std::string& descriptionPackageName,
            std::unordered_map<std::string, std::string>& meshUriMap,
            std::vector<std::filesystem::path>& generatedFiles)
        {
            std::unordered_map<std::string, std::size_t> fileNameCounts;
            for (const auto& mesh : project.MeshResources)
            {
                if (mesh.ResolvedPath.empty())
                {
                    continue;
                }
                const std::filesystem::path source(mesh.ResolvedPath);
                std::error_code ec;
                if (!std::filesystem::exists(source, ec))
                {
                    continue;
                }
                std::string fileName = source.filename().string();
                const std::size_t count = fileNameCounts[fileName]++;
                if (count > 0)
                {
                    fileName = source.stem().string() + "_" + std::to_string(count) + source.extension().string();
                }
                const std::filesystem::path destination = descriptionPackageRoot / "meshes" / "imported" / fileName;
                std::filesystem::create_directories(destination.parent_path());
                std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
                if (!ec)
                {
                    generatedFiles.push_back(destination);
                    meshUriMap[mesh.Uri] = "package://" + descriptionPackageName + "/meshes/imported/" + fileName;
                }
            }
        }

        void CopyOptionalDescriptionSources(
            const apex::io::StudioProject& project,
            const std::filesystem::path& descriptionPackageRoot,
            std::vector<std::filesystem::path>& generatedFiles)
        {
            const auto copyIfPresent = [&](const std::string& sourcePathValue, const std::filesystem::path& destinationDirectory)
            {
                if (sourcePathValue.empty())
                {
                    return;
                }
                const std::filesystem::path sourcePath(sourcePathValue);
                std::error_code ec;
                if (!std::filesystem::exists(sourcePath, ec))
                {
                    return;
                }
                const std::filesystem::path destination = destinationDirectory / sourcePath.filename();
                std::filesystem::create_directories(destination.parent_path());
                std::filesystem::copy_file(sourcePath, destination, std::filesystem::copy_options::overwrite_existing, ec);
                if (!ec)
                {
                    generatedFiles.push_back(destination);
                }
            };

            copyIfPresent(project.DescriptionSource.SourcePath, descriptionPackageRoot / "source");
            copyIfPresent(project.DescriptionSource.ExpandedDescriptionPath, descriptionPackageRoot / "source");
        }

        std::string BuildBringupPackageXml(const std::string& packageName, const std::string& descriptionPackage)
        {
            std::ostringstream builder;
            builder << R"(<package format="3">
  <name>)" << packageName << R"(</name>
  <version>0.1.0</version>
  <description>Apex Robotics Studio generated ROS 2 bringup package.</description>
  <maintainer email="demo@example.com">Apex Robotics Studio</maintainer>
  <license>MIT</license>

  <buildtool_depend>ament_cmake</buildtool_depend>
  <depend>rclcpp</depend>
  <depend>sensor_msgs</depend>
  <depend>std_msgs</depend>
  <exec_depend>robot_state_publisher</exec_depend>
  <exec_depend>)" << descriptionPackage << R"(</exec_depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
)";
            return builder.str();
        }

        std::string BuildBringupCMakeLists(const std::string& packageName)
        {
            std::ostringstream builder;
            builder << R"(cmake_minimum_required(VERSION 3.16)
project()" << packageName << R"(()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(apex_project_bridge_node src/apex_project_bridge_node.cpp)
ament_target_dependencies(apex_project_bridge_node rclcpp sensor_msgs std_msgs)

target_compile_features(apex_project_bridge_node PUBLIC cxx_std_17)

install(TARGETS apex_project_bridge_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})

ament_package()
)";
            return builder.str();
        }

        std::string BuildBringupNodeCpp(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder << R"(#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <std_msgs/msg/string.hpp>

#include <sstream>
#include <vector>

class ApexProjectBridgeNode final : public rclcpp::Node
{
public:
  ApexProjectBridgeNode()
  : rclcpp::Node("apex_project_bridge_node")
  {
    joint_names_ = this->declare_parameter<std::vector<std::string>>("joint_names", std::vector<std::string>{});
    joint_state_pub_ = this->create_publisher<sensor_msgs::msg::JointState>("joint_states", 10);
    summary_pub_ = this->create_publisher<std_msgs::msg::String>("apex_project_summary", 10);
    timer_ = this->create_wall_timer(std::chrono::milliseconds(500), std::bind(&ApexProjectBridgeNode::OnTimer, this));
  }

private:
  void OnTimer()
  {
    sensor_msgs::msg::JointState state;
    state.header.stamp = this->now();
    state.name = joint_names_;
    state.position.assign(joint_names_.size(), 0.0);
    joint_state_pub_->publish(state);

    std_msgs::msg::String summary;
    std::ostringstream stream;
    stream << "Project: )" << project.ProjectName << R"( | Robot: )" << project.RobotName << R"( | Obstacles: )" << project.Obstacles.size() << R"(";
    summary.data = stream.str();
    summary_pub_->publish(summary);
  }

  std::vector<std::string> joint_names_;
  rclcpp::Publisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr summary_pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ApexProjectBridgeNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
)";
            return builder.str();
        }

        std::string BuildBringupLaunchPy(const std::string& packageName, const std::string& descriptionPackage)
        {
            std::ostringstream builder;
            builder << R"(from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    config = os.path.join(get_package_share_directory(')" << packageName << R"('), 'config', 'project.yaml')
    display_launch = os.path.join(get_package_share_directory(')" << descriptionPackage << R"('), 'launch', 'display.launch.py')
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(display_launch)),
        Node(
            package='" << packageName << R"(',
            executable='apex_project_bridge_node',
            name='apex_project_bridge_node',
            parameters=[config],
            output='screen')
    ])
)";
            return builder.str();
        }

        std::string BuildMoveItPackageXml(const std::string& packageName, const std::string& descriptionPackage)
        {
            std::ostringstream builder;
            builder << R"(<package format="3">
  <name>)" << packageName << R"(</name>
  <version>0.1.0</version>
  <description>Apex Robotics Studio generated MoveIt 2 skeleton package.</description>
  <maintainer email="demo@example.com">Apex Robotics Studio</maintainer>
  <license>MIT</license>

  <buildtool_depend>ament_cmake</buildtool_depend>
  <depend>rclcpp</depend>
  <depend>moveit_ros_planning_interface</depend>
  <depend>moveit_visual_tools</depend>
  <depend>geometry_msgs</depend>
  <depend>moveit_msgs</depend>
  <depend>shape_msgs</depend>
  <depend>controller_manager</depend>
  <depend>joint_trajectory_controller</depend>
  <exec_depend>)" << descriptionPackage << R"(</exec_depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
)";
            return builder.str();
        }

        std::string BuildMoveItCMakeLists(const std::string& packageName)
        {
            std::ostringstream builder;
            builder << R"(cmake_minimum_required(VERSION 3.16)
project()" << packageName << R"(()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)
find_package(moveit_visual_tools REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(moveit_msgs REQUIRED)
find_package(shape_msgs REQUIRED)

add_executable(apex_moveit_smoke_test src/apex_moveit_smoke_test.cpp)
ament_target_dependencies(apex_moveit_smoke_test rclcpp moveit_ros_planning_interface moveit_visual_tools geometry_msgs moveit_msgs shape_msgs)

target_compile_features(apex_moveit_smoke_test PUBLIC cxx_std_17)

add_executable(apex_scene_loader_node src/apex_scene_loader_node.cpp)
ament_target_dependencies(apex_scene_loader_node rclcpp moveit_ros_planning_interface geometry_msgs moveit_msgs shape_msgs)

target_compile_features(apex_scene_loader_node PUBLIC cxx_std_17)

install(TARGETS apex_moveit_smoke_test apex_scene_loader_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch srdf rviz DESTINATION share/${PROJECT_NAME})

ament_package()
)";
            return builder.str();
        }

        std::string BuildMoveItTaskPackageXml(const std::string& packageName, const std::string& descriptionPackage, const std::string& moveitPackage)
        {
            std::ostringstream builder;
            builder << R"(<package format="3">
  <name>)" << packageName << R"(</name>
  <version>0.1.0</version>
  <description>Apex Robotics Studio generated MoveIt task/demo package.</description>
  <maintainer email="demo@example.com">Apex Robotics Studio</maintainer>
  <license>MIT</license>

  <buildtool_depend>ament_cmake</buildtool_depend>
  <depend>rclcpp</depend>
  <depend>moveit_ros_planning_interface</depend>
  <exec_depend>)" << descriptionPackage << R"(</exec_depend>
  <exec_depend>)" << moveitPackage << R"(</exec_depend>

  <export>
    <build_type>ament_cmake</build_type>
  </export>
</package>
)";
            return builder.str();
        }

        std::string BuildMoveItTaskCMakeLists(const std::string& packageName)
        {
            std::ostringstream builder;
            builder << R"(cmake_minimum_required(VERSION 3.16)
project()" << packageName << R"(()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(moveit_ros_planning_interface REQUIRED)

add_executable(apex_mtc_demo src/apex_mtc_demo.cpp)
ament_target_dependencies(apex_mtc_demo rclcpp moveit_ros_planning_interface)

target_compile_features(apex_mtc_demo PUBLIC cxx_std_17)

install(TARGETS apex_mtc_demo DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config launch DESTINATION share/${PROJECT_NAME})

ament_package()
)";
            return builder.str();
        }

        std::string BuildMoveItTaskDemoCpp(const std::string& planningGroup)
        {
            std::ostringstream builder;
            builder << R"(#include <moveit/move_group_interface/move_group_interface.h>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("apex_mtc_demo");
  moveit::planning_interface::MoveGroupInterface move_group(node, ")" << planningGroup << R"(");
  RCLCPP_INFO(node->get_logger(), "Generated MoveIt task/demo skeleton ready for integration.");
  RCLCPP_INFO(node->get_logger(), "Planning frame: %s", move_group.getPlanningFrame().c_str());
  rclcpp::shutdown();
  return 0;
}
)";
            return builder.str();
        }

        std::string BuildMoveItTaskLaunchPy(const std::string& packageName, const std::string& moveitPackage)
        {
            std::ostringstream builder;
            builder << R"(from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    moveit_launch = os.path.join(get_package_share_directory(')" << moveitPackage << R"('), 'launch', 'move_group.launch.py')
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(moveit_launch)),
        Node(package=')" << packageName << R"(', executable='apex_mtc_demo', output='screen')
    ])
)";
            return builder.str();
        }

        std::string BuildMoveItTaskParametersYaml(const std::string& planningGroup)
        {
            std::ostringstream builder;
            builder << "task_demo:\n"
                    << "  ros__parameters:\n"
                    << "    planning_group: \"" << planningGroup << "\"\n"
                    << "    scenario: \"generated_demo\"\n";
            return builder.str();
        }

        std::string BuildMoveItSmokeTestCpp(const std::string& planningGroup)
        {
            std::ostringstream builder;
            builder << R"(#include <moveit/move_group_interface/move_group_interface.h>
#include <moveit/planning_scene_interface/planning_scene_interface.hpp>
#include <moveit_msgs/msg/collision_object.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <rclcpp/rclcpp.hpp>

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("apex_moveit_smoke_test");

  moveit::planning_interface::MoveGroupInterface move_group(node, ")" << planningGroup << R"(");
  moveit::planning_interface::PlanningSceneInterface planning_scene_interface;

  moveit_msgs::msg::CollisionObject collision_object;
  collision_object.id = "apex_fixture";
  collision_object.header.frame_id = move_group.getPlanningFrame();
  planning_scene_interface.applyCollisionObject(collision_object);

  move_group.setPlanningTime(5.0);
  move_group.setGoalTolerance(0.01);
  move_group.setNamedTarget("ready");

  moveit::planning_interface::MoveGroupInterface::Plan plan;
  const bool success = static_cast<bool>(move_group.plan(plan));
  RCLCPP_INFO(node->get_logger(), "MoveIt smoke test plan result: %s", success ? "SUCCESS" : "FAILED");

  rclcpp::shutdown();
  return success ? 0 : 1;
}
)";
            return builder.str();
        }

        std::string BuildSceneLoaderNodeCpp(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder.setf(std::ios::fixed);
            builder.precision(6);
            builder << R"(#include <moveit/planning_scene_interface/planning_scene_interface.hpp>
#include <moveit_msgs/msg/collision_object.hpp>
#include <shape_msgs/msg/solid_primitive.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <rclcpp/rclcpp.hpp>

#include <vector>

class ApexSceneLoaderNode final : public rclcpp::Node
{
public:
  ApexSceneLoaderNode()
  : rclcpp::Node("apex_scene_loader_node")
  {
    timer_ = this->create_wall_timer(std::chrono::milliseconds(250), std::bind(&ApexSceneLoaderNode::PublishScene, this));
  }

private:
  void PublishScene()
  {
    if (published_)
    {
      return;
    }
    moveit::planning_interface::PlanningSceneInterface planning_scene_interface;
    std::vector<moveit_msgs::msg::CollisionObject> collision_objects;
)";
            for (const auto& obstacle : project.Obstacles)
            {
                const double sizeX = obstacle.Bounds.Max.X - obstacle.Bounds.Min.X;
                const double sizeY = obstacle.Bounds.Max.Y - obstacle.Bounds.Min.Y;
                const double sizeZ = obstacle.Bounds.Max.Z - obstacle.Bounds.Min.Z;
                const double centerX = (obstacle.Bounds.Min.X + obstacle.Bounds.Max.X) * 0.5;
                const double centerY = (obstacle.Bounds.Min.Y + obstacle.Bounds.Max.Y) * 0.5;
                const double centerZ = (obstacle.Bounds.Min.Z + obstacle.Bounds.Max.Z) * 0.5;
                builder << "    {\n"
                        << "      moveit_msgs::msg::CollisionObject object;\n"
                        << "      object.header.frame_id = \"base_link\";\n"
                        << "      object.id = \"" << obstacle.Name << "\";\n"
                        << "      shape_msgs::msg::SolidPrimitive primitive;\n"
                        << "      primitive.type = primitive.BOX;\n"
                        << "      primitive.dimensions = {" << sizeX << ", " << sizeY << ", " << sizeZ << "};\n"
                        << "      geometry_msgs::msg::Pose pose;\n"
                        << "      pose.orientation.w = 1.0;\n"
                        << "      pose.position.x = " << centerX << ";\n"
                        << "      pose.position.y = " << centerY << ";\n"
                        << "      pose.position.z = " << centerZ << ";\n"
                        << "      object.primitives.push_back(primitive);\n"
                        << "      object.primitive_poses.push_back(pose);\n"
                        << "      object.operation = object.ADD;\n"
                        << "      collision_objects.push_back(object);\n"
                        << "    }\n";
            }
            builder << R"(
    planning_scene_interface.applyCollisionObjects(collision_objects);
    RCLCPP_INFO(this->get_logger(), "Loaded %zu collision objects into the planning scene.", collision_objects.size());
    published_ = true;
  }

  bool published_ = false;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<ApexSceneLoaderNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
)";
            return builder.str();
        }

        std::string BuildMoveItRvizConfig(const std::string& planningGroup)
        {
            std::ostringstream builder;
            builder << "Panels:\n"
                    << "  - Class: rviz_common/Displays\n"
                    << "Visualization Manager:\n"
                    << "  Class: \"\"\n"
                    << "  Displays:\n"
                    << "    - Class: rviz_default_plugins/Grid\n"
                    << "      Name: Grid\n"
                    << "    - Class: moveit_rviz_plugin/MotionPlanning\n"
                    << "      Name: Motion Planning\n"
                    << "      Planning Group: " << planningGroup << "\n";
            return builder.str();
        }

        std::string BuildMoveItLaunchPy(const std::string& packageName, const std::string& descriptionPackage)
        {
            std::ostringstream builder;
            builder << R"(from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    display_launch = os.path.join(get_package_share_directory(')" << descriptionPackage << R"('), 'launch', 'display.launch.py')
    moveit_rviz = os.path.join(get_package_share_directory(')" << packageName << R"('), 'rviz', 'moveit_demo.rviz')
    return LaunchDescription([
        IncludeLaunchDescription(PythonLaunchDescriptionSource(display_launch)),
        Node(
            package='" << packageName << R"(',
            executable='apex_scene_loader_node',
            output='screen'),
        Node(
            package='" << packageName << R"(',
            executable='apex_moveit_smoke_test',
            output='screen'),
        Node(
            package='rviz2',
            executable='rviz2',
            arguments=['-d', moveit_rviz],
            output='screen')
    ])
)";
            return builder.str();
        }

        std::string BuildJointLimitsYaml(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder << "joint_limits:\n";
            for (const auto& joint : project.Joints)
            {
                builder << "  " << joint.Name << ":\n"
                        << "    has_velocity_limits: true\n"
                        << "    max_velocity: 1.5\n"
                        << "    has_acceleration_limits: false\n";
            }
            return builder.str();
        }

        std::string BuildKinematicsYaml(const std::string& planningGroup)
        {
            std::ostringstream builder;
            builder << planningGroup << ":\n"
                    << "  kinematics_solver: kdl_kinematics_plugin/KDLKinematicsPlugin\n"
                    << "  kinematics_solver_search_resolution: 0.005\n"
                    << "  kinematics_solver_timeout: 0.05\n";
            return builder.str();
        }

        std::string BuildOmplYaml(const std::string& planningGroup)
        {
            std::ostringstream builder;
            builder << planningGroup << ":\n"
                    << "  planner_configs:\n"
                    << "    - RRTConnectkConfigDefault\n";
            return builder.str();
        }

        std::string BuildMoveItControllersYaml(const apex::io::StudioProject& project)
        {
            std::ostringstream builder;
            builder << "moveit_controller_manager: moveit_simple_controller_manager/MoveItSimpleControllerManager\n"
                    << "moveit_simple_controller_manager:\n"
                    << "  controller_names:\n"
                    << "    - arm_controller\n"
                    << "  arm_controller:\n"
                    << "    type: FollowJointTrajectory\n"
                    << "    joints:\n";
            for (const auto& joint : project.Joints)
            {
                builder << "      - " << joint.Name << "\n";
            }
            return builder.str();
        }

        std::string BuildSrdfSkeleton(const apex::io::StudioProject& project, const std::string& planningGroup)
        {
            std::ostringstream builder;
            builder << "<robot name=\"" << project.RobotName << "\">\n"
                    << "  <group name=\"" << planningGroup << "\">\n";
            for (const auto& joint : project.Joints)
            {
                builder << "    <joint name=\"" << joint.Name << "\"/>\n";
            }
            builder << "  </group>\n"
                    << "  <group_state name=\"ready\" group=\"" << planningGroup << "\">\n";
            for (const auto& joint : project.Joints)
            {
                builder << "    <joint name=\"" << joint.Name << "\" value=\"0\"/>\n";
            }
            builder << "  </group_state>\n"
                    << "</robot>\n";
            return builder.str();
        }
    }

    Ros2WorkspaceExportResult Ros2WorkspaceExporter::ExportWorkspace(
        const apex::io::StudioProject& project,
        const std::filesystem::path& outputDirectory,
        const Ros2WorkspaceExportOptions& options) const
    {
        const std::string packageStem = SanitizeToken(options.PackageStem.empty() ? project.ProjectName : options.PackageStem);
        const std::string descriptionPackage = packageStem + "_description";
        const std::string bringupPackage = packageStem + "_bringup";
        const std::string moveitPackage = packageStem + "_moveit_config";
        const std::string mtcPackage = packageStem + "_moveit_task_demo";

        Ros2WorkspaceExportResult result;
        result.WorkspaceRoot = outputDirectory / options.WorkspaceName;
        const std::filesystem::path srcRoot = result.WorkspaceRoot / "src";
        result.DescriptionPackageRoot = srcRoot / descriptionPackage;
        result.BringupPackageRoot = srcRoot / bringupPackage;
        result.MoveItPackageRoot = srcRoot / moveitPackage;
        result.MoveItTaskPackageRoot = srcRoot / mtcPackage;
        result.DescriptionPackageName = descriptionPackage;
        result.MoveItTaskPackageName = mtcPackage;

        std::unordered_map<std::string, std::string> meshUriMap;
        if (options.IncludeDescriptionPackage)
        {
            WriteTextFile(result.DescriptionPackageRoot / "package.xml", BuildDescriptionPackageXml(descriptionPackage));
            WriteTextFile(result.DescriptionPackageRoot / "CMakeLists.txt", BuildDescriptionCMakeLists(descriptionPackage));
            WriteTextFile(result.DescriptionPackageRoot / "rviz/view_robot.rviz", BuildDefaultRviz());
            WriteTextFile(result.DescriptionPackageRoot / "config/ros2_controllers.yaml", BuildRos2ControllersYaml(project));
            WriteTextFile(result.DescriptionPackageRoot / "config/initial_positions.yaml", BuildInitialPositionsYaml(project));
            WriteTextFile(result.DescriptionPackageRoot / "config/package_dependencies.yaml", BuildPackageDependenciesYaml(project));
            WriteTextFile(result.DescriptionPackageRoot / "config/resource_manifest.json", BuildResourceManifestJson(project));
            if (options.CopyMeshResources)
            {
                CopyMeshResources(project, result.DescriptionPackageRoot, descriptionPackage, meshUriMap, result.GeneratedFiles);
            }
            CopyOptionalDescriptionSources(project, result.DescriptionPackageRoot, result.GeneratedFiles);
            const std::string generatedUrdf = BuildGeneratedUrdf(project, descriptionPackage, options.RewriteMeshUris ? meshUriMap : std::unordered_map<std::string, std::string>{});
            const std::string urdfFileName = SanitizeToken(project.RobotName) + ".urdf";
            WriteTextFile(result.DescriptionPackageRoot / "urdf" / urdfFileName, generatedUrdf);
            WriteTextFile(result.DescriptionPackageRoot / "launch/display.launch.py", BuildDescriptionDisplayLaunchPy(descriptionPackage, urdfFileName));
            WriteTextFile(result.DescriptionPackageRoot / "launch/controllers.launch.py", BuildControllersLaunchPy(descriptionPackage, "ros2_controllers.yaml"));
            WriteTextFile(result.DescriptionPackageRoot / "README.md", std::string("# ") + descriptionPackage + "\n\nGenerated by Apex Robotics Studio.\n");
            WriteTextFile(result.DescriptionPackageRoot / "resource" / descriptionPackage, "");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "package.xml");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "CMakeLists.txt");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "urdf" / urdfFileName);
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "launch/display.launch.py");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "launch/controllers.launch.py");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "rviz/view_robot.rviz");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "config/ros2_controllers.yaml");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "config/initial_positions.yaml");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "config/package_dependencies.yaml");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "config/resource_manifest.json");
            result.GeneratedFiles.push_back(result.DescriptionPackageRoot / "README.md");
        }

        WriteTextFile(result.BringupPackageRoot / "package.xml", BuildBringupPackageXml(bringupPackage, descriptionPackage));
        WriteTextFile(result.BringupPackageRoot / "CMakeLists.txt", BuildBringupCMakeLists(bringupPackage));
        WriteTextFile(result.BringupPackageRoot / "src/apex_project_bridge_node.cpp", BuildBringupNodeCpp(project));
        WriteTextFile(result.BringupPackageRoot / "launch/bringup.launch.py", BuildBringupLaunchPy(bringupPackage, descriptionPackage));
        WriteTextFile(result.BringupPackageRoot / "config/project.yaml", BuildProjectConfigYaml(project, descriptionPackage));
        WriteTextFile(result.BringupPackageRoot / "resource" / bringupPackage, "");
        result.GeneratedFiles.push_back(result.BringupPackageRoot / "package.xml");
        result.GeneratedFiles.push_back(result.BringupPackageRoot / "CMakeLists.txt");
        result.GeneratedFiles.push_back(result.BringupPackageRoot / "src/apex_project_bridge_node.cpp");
        result.GeneratedFiles.push_back(result.BringupPackageRoot / "launch/bringup.launch.py");
        result.GeneratedFiles.push_back(result.BringupPackageRoot / "config/project.yaml");

        if (options.IncludeMoveItSkeleton)
        {
            WriteTextFile(result.MoveItPackageRoot / "package.xml", BuildMoveItPackageXml(moveitPackage, descriptionPackage));
            WriteTextFile(result.MoveItPackageRoot / "CMakeLists.txt", BuildMoveItCMakeLists(moveitPackage));
            WriteTextFile(result.MoveItPackageRoot / "src/apex_moveit_smoke_test.cpp", BuildMoveItSmokeTestCpp(options.PlanningGroupName));
            WriteTextFile(result.MoveItPackageRoot / "src/apex_scene_loader_node.cpp", BuildSceneLoaderNodeCpp(project));
            WriteTextFile(result.MoveItPackageRoot / "launch/demo_moveit.launch.py", BuildMoveItLaunchPy(moveitPackage, descriptionPackage));
            WriteTextFile(result.MoveItPackageRoot / "launch/move_group.launch.py", BuildMoveGroupLaunchPy(moveitPackage, descriptionPackage));
            WriteTextFile(result.MoveItPackageRoot / "launch/bringup_with_moveit.launch.py", BuildBringupWithMoveItLaunchPy(bringupPackage, moveitPackage));
            WriteTextFile(result.MoveItPackageRoot / "launch/full_stack_demo.launch.py", BuildFullStackDemoLaunchPy(bringupPackage, moveitPackage, mtcPackage, options.IncludeMoveItTaskSkeleton));
            WriteTextFile(result.MoveItPackageRoot / "config/joint_limits.yaml", BuildJointLimitsYaml(project));
            WriteTextFile(result.MoveItPackageRoot / "config/kinematics.yaml", BuildKinematicsYaml(options.PlanningGroupName));
            WriteTextFile(result.MoveItPackageRoot / "config/ompl_planning.yaml", BuildOmplYaml(options.PlanningGroupName));
            WriteTextFile(result.MoveItPackageRoot / "config/moveit_controllers.yaml", BuildMoveItControllersYaml(project));
            WriteTextFile(result.MoveItPackageRoot / "config/scene_objects.yaml", BuildSceneObjectsYaml(project));
            WriteTextFile(result.MoveItPackageRoot / "config/moveit_cpp.yaml", BuildMoveItCppYaml(options.PlanningGroupName));
            WriteTextFile(result.MoveItPackageRoot / "config/planning_scene_monitor_parameters.yaml", BuildPlanningSceneMonitorParametersYaml());
            WriteTextFile(result.MoveItPackageRoot / "config/initial_positions.yaml", BuildInitialPositionsYaml(project));
            WriteTextFile(result.MoveItPackageRoot / "rviz/moveit_demo.rviz", BuildMoveItRvizConfig(options.PlanningGroupName));
            if (options.IncludeRos2ControlSkeleton)
            {
                WriteTextFile(result.MoveItPackageRoot / "config/ros2_controllers.yaml", BuildRos2ControllersYaml(project));
            }
            WriteTextFile(result.MoveItPackageRoot / "srdf/apex_generated.srdf", BuildSrdfSkeleton(project, options.PlanningGroupName));
            WriteTextFile(result.MoveItPackageRoot / "resource" / moveitPackage, "");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "package.xml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "CMakeLists.txt");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "src/apex_moveit_smoke_test.cpp");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "src/apex_scene_loader_node.cpp");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "launch/move_group.launch.py");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "launch/bringup_with_moveit.launch.py");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "launch/full_stack_demo.launch.py");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "config/joint_limits.yaml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "config/kinematics.yaml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "config/ompl_planning.yaml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "config/moveit_controllers.yaml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "config/scene_objects.yaml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "config/moveit_cpp.yaml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "config/planning_scene_monitor_parameters.yaml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "config/initial_positions.yaml");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "rviz/moveit_demo.rviz");
            result.GeneratedFiles.push_back(result.MoveItPackageRoot / "srdf/apex_generated.srdf");
        }

        if (options.IncludeMoveItSkeleton && options.IncludeMoveItTaskSkeleton)
        {
            WriteTextFile(result.MoveItTaskPackageRoot / "package.xml", BuildMoveItTaskPackageXml(mtcPackage, descriptionPackage, moveitPackage));
            WriteTextFile(result.MoveItTaskPackageRoot / "CMakeLists.txt", BuildMoveItTaskCMakeLists(mtcPackage));
            WriteTextFile(result.MoveItTaskPackageRoot / "src/apex_mtc_demo.cpp", BuildMoveItTaskDemoCpp(options.PlanningGroupName));
            WriteTextFile(result.MoveItTaskPackageRoot / "launch/mtc_demo.launch.py", BuildMoveItTaskLaunchPy(mtcPackage, moveitPackage));
            WriteTextFile(result.MoveItTaskPackageRoot / "config/task_parameters.yaml", BuildMoveItTaskParametersYaml(options.PlanningGroupName));
            WriteTextFile(result.MoveItTaskPackageRoot / "resource" / mtcPackage, "");
            result.GeneratedFiles.push_back(result.MoveItTaskPackageRoot / "package.xml");
            result.GeneratedFiles.push_back(result.MoveItTaskPackageRoot / "CMakeLists.txt");
            result.GeneratedFiles.push_back(result.MoveItTaskPackageRoot / "src/apex_mtc_demo.cpp");
            result.GeneratedFiles.push_back(result.MoveItTaskPackageRoot / "launch/mtc_demo.launch.py");
            result.GeneratedFiles.push_back(result.MoveItTaskPackageRoot / "config/task_parameters.yaml");
        }

        WriteTextFile(result.WorkspaceRoot / "README.md", BuildWorkspaceReadme(project, descriptionPackage, bringupPackage, moveitPackage, mtcPackage, options.IncludeMoveItSkeleton, options.IncludeMoveItSkeleton && options.IncludeMoveItTaskSkeleton));
        WriteTextFile(result.WorkspaceRoot / ".gitignore", BuildWorkspaceGitIgnore());
        WriteTextFile(result.WorkspaceRoot / "colcon.meta", BuildWorkspaceColconMeta(descriptionPackage, bringupPackage, moveitPackage, mtcPackage, options.IncludeMoveItSkeleton && options.IncludeMoveItTaskSkeleton));
        WriteTextFile(result.WorkspaceRoot / "workspace_manifest.json", BuildWorkspaceManifestJson(project, descriptionPackage, bringupPackage, moveitPackage, mtcPackage, options.IncludeMoveItSkeleton, options.IncludeMoveItSkeleton && options.IncludeMoveItTaskSkeleton));
        WriteTextFile(result.WorkspaceRoot / "dependency_catalog.md", BuildDependencyCatalogMarkdown(project));
        WriteTextFile(result.WorkspaceRoot / "workspace_doctor.py", BuildWorkspaceDoctorPy(descriptionPackage, bringupPackage, moveitPackage, mtcPackage, options.IncludeMoveItSkeleton, options.IncludeMoveItSkeleton && options.IncludeMoveItTaskSkeleton));
        result.GeneratedFiles.push_back(result.WorkspaceRoot / "README.md");
        result.GeneratedFiles.push_back(result.WorkspaceRoot / ".gitignore");
        result.GeneratedFiles.push_back(result.WorkspaceRoot / "colcon.meta");
        result.GeneratedFiles.push_back(result.WorkspaceRoot / "workspace_manifest.json");
        result.GeneratedFiles.push_back(result.WorkspaceRoot / "dependency_catalog.md");
        result.GeneratedFiles.push_back(result.WorkspaceRoot / "workspace_doctor.py");
        return result;
    }
}
