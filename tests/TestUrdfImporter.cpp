#include "TestFramework.h"
#include "apex/importer/UrdfImporter.h"

#include <filesystem>
#include <cstdlib>
#include <fstream>

ARS_TEST(TestUrdfImporterLoadsRevoluteJoints)
{
    const std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "apex_test_robot.urdf";
    std::ofstream stream(tempPath);
    stream << R"(<robot name="UnitTestRobot">
  <joint name="joint_1" type="revolute">
    <origin xyz="0.40 0 0" rpy="0 0 0"/>
    <limit lower="-1.57" upper="1.57" effort="10" velocity="1"/>
  </joint>
  <joint name="joint_2" type="continuous">
    <origin xyz="0.30 0 0" rpy="0 0 0"/>
  </joint>
  <joint name="fixed_mount" type="fixed">
    <origin xyz="0.10 0 0" rpy="0 0 0"/>
  </joint>
</robot>)";
    stream.close();

    const apex::importer::UrdfImporter importer;
    const auto result = importer.ImportFromFile(tempPath);

    apex::tests::Require(result.ImportedRevoluteJointCount == 2, "Two supported joints should be imported");
    apex::tests::Require(result.Project.Joints.size() == 2, "Project should contain two joints");
    apex::tests::Require(result.Robot.GetName() == "UnitTestRobot", "Robot name should come from URDF");

    std::error_code errorCode;
    std::filesystem::remove(tempPath, errorCode);
}


ARS_TEST(TestUrdfImporterResolvesPackageXmlAndEnvStyleXacroTokens)
{
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "apex_test_xacro_pkg_v14";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot / "workspace/src/demo_description_pkg/meshes");
    std::filesystem::create_directories(tempRoot / "workspace/src/demo_description_pkg/urdf");

    {
        std::ofstream packageXml(tempRoot / "workspace/src/demo_description_pkg/package.xml");
        packageXml << R"xml(<package format="3"><name>demo_description_pkg</name><version>0.0.1</version></package>)xml";
    }
    {
        std::ofstream meshFile(tempRoot / "workspace/src/demo_description_pkg/meshes/tool.stl");
        meshFile << "solid tool\nendsolid tool\n";
    }
    {
        std::ofstream includeFile(tempRoot / "workspace/src/demo_description_pkg/urdf/tool_link.xacro");
        includeFile << R"xml(<robot xmlns:xacro="http://ros.org/wiki/xacro">
  <link name="tool_link">
    <visual>
      <geometry>
        <mesh filename="package://demo_description_pkg/meshes/tool.stl" scale="1 1 1"/>
      </geometry>
    </visual>
  </link>
</robot>)xml";
    }
    {
        std::ofstream rootFile(tempRoot / "robot.xacro");
        rootFile << R"xml(<robot xmlns:xacro="http://ros.org/wiki/xacro" name="$(arg robot_name)">
  <xacro:arg name="robot_name" default="$(optenv APEX_TEST_ROBOT_NAME DemoFromOptEnv)"/>
  <xacro:property name="mesh_dir" value="$(find demo_description_pkg)/meshes"/>
  <link name="base_link">
    <visual>
      <geometry>
        <mesh filename="$(dirname)/local_mesh.stl" scale="1 1 1"/>
      </geometry>
    </visual>
  </link>
  <xacro:include filename="$(find demo_description_pkg)/urdf/tool_link.xacro"/>
  <joint name="joint_1" type="revolute">
    <origin xyz="0.25 0 0" rpy="0 0 0"/>
    <limit lower="-1.0" upper="1.0" effort="10" velocity="1"/>
  </joint>
</robot>)xml";
    }
    {
        std::ofstream localMesh(tempRoot / "local_mesh.stl");
        localMesh << "solid local\nendsolid local\n";
    }

#if defined(_WIN32)
    _putenv_s("APEX_TEST_ROBOT_NAME", "EnvResolvedRobot");
    _putenv_s("ROS_PACKAGE_PATH", (tempRoot / "workspace/src").string().c_str());
#else
    setenv("APEX_TEST_ROBOT_NAME", "EnvResolvedRobot", 1);
    setenv("ROS_PACKAGE_PATH", (tempRoot / "workspace/src").string().c_str(), 1);
#endif

    apex::importer::UrdfImportOptions options;
    options.PreferExternalXacroTool = false;
    const apex::importer::UrdfImporter importer;
    const auto result = importer.ImportFromFile(tempRoot / "robot.xacro", options);

    apex::tests::Require(result.UsedXacro, "Importer should detect Xacro input");
    apex::tests::Require(result.Project.RobotName == "EnvResolvedRobot", "$(optenv ...) should resolve environment variable");
    apex::tests::Require(result.Project.MeshResources.size() >= 2, "Both local and package meshes should be captured");

    bool foundLocal = false;
    bool foundPackage = false;
    for (const auto& mesh : result.Project.MeshResources)
    {
        if (mesh.Uri.find("local_mesh.stl") != std::string::npos)
        {
            foundLocal = mesh.ResolvedPath.find("local_mesh.stl") != std::string::npos;
        }
        if (mesh.Uri.find("package://demo_description_pkg/meshes/tool.stl") != std::string::npos)
        {
            foundPackage = mesh.PackageName == "demo_description_pkg" && mesh.ResolvedPath.find("tool.stl") != std::string::npos;
        }
    }
    apex::tests::Require(foundLocal, "$(dirname) mesh URI should resolve to a local mesh path");
    apex::tests::Require(foundPackage, "Package mesh URI should resolve through package.xml discovery");
}
