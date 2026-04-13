#include "TestFramework.h"
#include "apex/importer/UrdfImporter.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    bool SetEnvVarForTest(const char* pName, const char* pValue)
    {
#if defined(_WIN32)
        return ::_putenv_s(pName, pValue) == 0;
#else
        return ::setenv(pName, pValue, 1) == 0;
#endif
    }

    bool UnsetEnvVarForTest(const char* pName)
    {
#if defined(_WIN32)
        return ::_putenv_s(pName, "") == 0;
#else
        return ::unsetenv(pName) == 0;
#endif
    }
}



ARS_TEST(TestRobotDescriptionImportSupportsXacroConditionalsAndEnvironmentFunctions)
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "apex_robot_description_import_v15";
    std::filesystem::remove_all(root);
    const std::filesystem::path packagesRoot = root / "workspace" / "src";
    const std::filesystem::path supportPackage = packagesRoot / "demo_support";
    const std::filesystem::path supportUrdfDir = supportPackage / "urdf";
    const std::filesystem::path supportMeshDir = supportPackage / "meshes";
    std::filesystem::create_directories(supportUrdfDir);
    std::filesystem::create_directories(supportMeshDir);

    {
        std::ofstream manifest(supportPackage / "package.xml");
        manifest << R"(<package format="3"><name>demo_support</name><version>0.0.1</version><description>demo</description><maintainer email="demo@example.com">demo</maintainer><license>MIT</license></package>)";
    }
    {
        std::ofstream mesh(supportMeshDir / "tool.stl");
        mesh << "solid tool\nendsolid tool\n";
    }
    {
        std::ofstream include(supportUrdfDir / "common.xacro");
        include << R"(<robot xmlns:xacro="http://www.ros.org/wiki/xacro">
          <xacro:macro name="tool_joint" params="joint_name joint_length enable_mesh">
            <joint name="${joint_name}" type="revolute">
              <origin xyz="${joint_length} 0 0" rpy="0 0 0"/>
              <limit lower="-1.57" upper="1.57" effort="10" velocity="1"/>
            </joint>
            <xacro:if value="${enable_mesh}">
              <link name="${joint_name}_tool">
                <visual>
                  <geometry>
                    <mesh filename="package://demo_support/meshes/tool.stl" scale="1 1 1"/>
                  </geometry>
                </visual>
              </link>
            </xacro:if>
            <xacro:unless value="${enable_mesh}">
              <link name="${joint_name}_placeholder"/>
            </xacro:unless>
          </xacro:macro>
        </robot>)";
    }

    const std::filesystem::path xacroPath = root / "robot.xacro";
    {
        std::ofstream stream(xacroPath);
        stream << R"XACRO(<robot xmlns:xacro="http://www.ros.org/wiki/xacro" name="DemoXacroV15">
          <xacro:arg name="joint_length" default="0.41"/>
          <xacro:arg name="enable_mesh" default="true"/>
          <xacro:include filename="$(find demo_support)/urdf/common.xacro"/>
          <link name="base_link"/>
          <xacro:property name="root_dir" value="$(dirname)"/>
          <xacro:if value="$(optenv APEX_ENABLE_SECOND_JOINT false)">
            <xacro:tool_joint joint_name="joint_2" joint_length="$(eval joint_length * 2.0)" enable_mesh="$(arg enable_mesh)"/>
          </xacro:if>
          <xacro:unless value="$(env APEX_DISABLE_FIRST_JOINT)">
            <xacro:tool_joint joint_name="joint_1" joint_length="${joint_length}" enable_mesh="$(arg enable_mesh)"/>
          </xacro:unless>
        </robot>)XACRO";
    }

    //setenv("APEX_ENABLE_SECOND_JOINT", "1", 1);
    //unsetenv("APEX_DISABLE_FIRST_JOINT");

    SetEnvVarForTest("APEX_ENABLE_SECOND_JOINT", "1");
    UnsetEnvVarForTest("APEX_DISABLE_FIRST_JOINT");


    apex::importer::UrdfImportOptions options;
    options.PreferExternalXacroTool = false;
    options.PackageSearchRoots = {packagesRoot};
    options.XacroArguments["joint_length"] = "0.33";
    options.XacroArguments["enable_mesh"] = "true";

    const apex::importer::UrdfImporter importer;
    const auto result = importer.ImportFromFile(xacroPath, options);

    apex::tests::Require(result.UsedXacro, "Importer should treat .xacro input as xacro");
    apex::tests::Require(result.ImportedRevoluteJointCount == 2, "Both conditional joints should be imported");
    apex::tests::Require(result.Project.MeshResources.size() == 2, "Both conditionally included mesh resources should be captured");
    apex::tests::Require(result.Project.MeshResources.front().PackageName == "demo_support", "Package manifest discovery should resolve package:// URI");
    apex::tests::Require(!result.Project.DescriptionSource.PackageDependencies.empty(), "Description package dependencies should be captured");
    apex::tests::Require(result.Project.DescriptionSource.PackageDependencies.front() == "demo_support", "Description dependency list should include support package");
    apex::tests::Require(result.ExpandedRobotXml.find("joint_2") != std::string::npos, "Expanded XML should contain the joint enabled by optenv");
    apex::tests::Require(result.ExpandedRobotXml.find("placeholder") == std::string::npos, "False xacro:unless branch should be removed after evaluation");
    apex::tests::RequireNear(result.Project.Joints.front().LinkLength, 0.66, 1e-6, "First imported joint should preserve expanded expression value");
    apex::tests::RequireNear(result.Project.Joints.back().LinkLength, 0.33, 1e-6, "Second imported joint should preserve arg override");
}
