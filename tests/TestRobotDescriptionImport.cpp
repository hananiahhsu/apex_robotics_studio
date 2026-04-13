#include "TestFramework.h"
#include "apex/importer/UrdfImporter.h"
#include "apex/io/StudioProjectSerializer.h"

#include <filesystem>
#include <fstream>

ARS_TEST(TestRobotDescriptionImportSupportsXacroArgsPackageFindAndMeshes)
{
    const std::filesystem::path root = std::filesystem::temp_directory_path() / "apex_robot_description_import_v13";
    std::filesystem::remove_all(root);
    const std::filesystem::path packagesRoot = root / "packages";
    const std::filesystem::path supportPackage = packagesRoot / "demo_support" / "urdf";
    const std::filesystem::path descriptionPackage = packagesRoot / "demo_description" / "meshes";
    std::filesystem::create_directories(supportPackage);
    std::filesystem::create_directories(descriptionPackage);

    const std::filesystem::path includePath = supportPackage / "arm_macro.xacro";
    const std::filesystem::path meshPath = descriptionPackage / "base.dae";
    const std::filesystem::path xacroPath = root / "robot.xacro";

    {
        std::ofstream includeStream(includePath);
        includeStream << R"(<robot xmlns:xacro="http://www.ros.org/wiki/xacro">
  <xacro:macro name="arm_joint" params="joint_name length">
    <joint name="${joint_name}" type="revolute">
      <origin xyz="${length} 0 0" rpy="0 0 0"/>
      <limit lower="-1.0" upper="1.0" effort="10" velocity="1"/>
    </joint>
  </xacro:macro>
</robot>)";
    }

    {
        std::ofstream meshStream(meshPath);
        meshStream << "dummy mesh";
    }

    {
        std::ofstream stream(xacroPath);
        stream << R"XACRO(<robot xmlns:xacro="http://www.ros.org/wiki/xacro" name="DemoXacroV13">
  <xacro:arg name="link_len" default="0.42"/>
  <xacro:include filename="$(find demo_support)/urdf/arm_macro.xacro"/>
  <link name="base_link">
    <visual><geometry><mesh filename="package://demo_description/meshes/base.dae" scale="1 1 1"/></geometry></visual>
  </link>
  <xacro:arm_joint joint_name="joint_1" length="$(arg link_len)"/>
</robot>)XACRO";
    }

    apex::importer::UrdfImportOptions options;
    options.PreferExternalXacroTool = false;
    options.PackageSearchRoots = {packagesRoot};
    options.XacroArguments["link_len"] = "0.57";

    const apex::importer::UrdfImporter importer;
    const auto result = importer.ImportFromFile(xacroPath, options);

    apex::tests::Require(result.UsedXacro, "Importer should detect xacro input");
    apex::tests::Require(result.ImportedRevoluteJointCount == 1, "One revolute joint should be imported from xacro macro");
    apex::tests::Require(result.Project.MeshResources.size() == 1, "One mesh resource should be captured");
    apex::tests::Require(result.ResolvedMeshResourceCount == 1, "One mesh resource should be resolved to a concrete file path");
    apex::tests::Require(result.Project.MeshResources.front().PackageName == "demo_description", "Package name should be captured from package URI");
    apex::tests::Require(result.Project.MeshResources.front().ResolvedPath == std::filesystem::weakly_canonical(meshPath).string(), "Mesh resolved path should match actual package mesh file");
    apex::tests::Require(result.Project.DescriptionSource.SourceKind == "xacro", "Description source kind should be xacro");
    apex::tests::RequireNear(result.Project.Joints.front().LinkLength, 0.57, 1e-6, "Link length should come from xacro arg override");

    const apex::io::StudioProjectSerializer serializer;
    const std::string roundTrip = serializer.SaveToString(result.Project);
    const auto loaded = serializer.LoadFromString(roundTrip);
    apex::tests::Require(loaded.SchemaVersion == 5, "Resolved mesh metadata should upgrade schema version to 5");
    apex::tests::Require(loaded.MeshResources.size() == 1, "Mesh resources should survive project round-trip");
    apex::tests::Require(loaded.MeshResources.front().ResolvedPath == std::filesystem::weakly_canonical(meshPath).string(), "Resolved mesh path should survive round-trip");
}
