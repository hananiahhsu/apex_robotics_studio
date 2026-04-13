#include "TestFramework.h"
#include "apex/importer/RobotDescriptionInspector.h"

#include <filesystem>
#include <fstream>
#include <iterator>

ARS_TEST(TestRobotDescriptionInspectorSummarizesDependenciesAndMeshes)
{
    apex::io::StudioProject project;
    project.ProjectName = "Inspection Demo";
    project.RobotName = "InspectorBot";
    project.DescriptionSource.SourceKind = "xacro";
    project.DescriptionSource.SourcePath = "/tmp/robot.xacro";
    project.DescriptionSource.ExpandedDescriptionPath = "/tmp/robot.expanded.urdf";
    project.DescriptionSource.PackageDependencies = {"vendor_pkg", "support_pkg"};

    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "apex_robot_description_inspector_v17";
    std::filesystem::remove_all(tempRoot);
    std::filesystem::create_directories(tempRoot);
    const std::filesystem::path resolvedMesh = tempRoot / "tool.stl";
    {
        std::ofstream stream(resolvedMesh);
        stream << "solid tool\nendsolid tool\n";
    }

    project.MeshResources.push_back({"tool_link", "visual_mesh", "package://vendor_pkg/meshes/tool.stl", resolvedMesh.string(), "vendor_pkg", {1.0, 1.0, 1.0}});
    project.MeshResources.push_back({"tool_link_duplicate", "collision_mesh", "package://vendor_pkg/meshes/tool.stl", resolvedMesh.string(), "vendor_pkg", {1.0, 1.0, 1.0}});
    project.MeshResources.push_back({"missing_link", "visual_mesh", "package://support_pkg/meshes/missing.stl", std::string{}, "support_pkg", {1.0, 1.0, 1.0}});

    const apex::importer::RobotDescriptionInspector inspector;
    const auto inspection = inspector.Inspect(project);

    apex::tests::Require(inspection.PackageDependencyCount == 2, "Inspector should count imported package dependencies");
    apex::tests::Require(inspection.MeshResourceCount == 3, "Inspector should count mesh resources");
    apex::tests::Require(inspection.ResolvedMeshCount == 2, "Inspector should count resolved mesh paths");
    apex::tests::Require(inspection.MissingMeshCount == 1, "Inspector should detect unresolved meshes");
    apex::tests::Require(!inspection.DuplicateMeshUris.empty(), "Inspector should detect duplicate mesh URIs");

    const std::filesystem::path jsonPath = tempRoot / "inspection.json";
    const std::filesystem::path htmlPath = tempRoot / "inspection.html";
    inspector.WriteJsonReport(inspection, jsonPath);
    inspector.WriteHtmlReport(inspection, htmlPath);

    apex::tests::Require(std::filesystem::exists(jsonPath), "JSON inspection report should be generated");
    apex::tests::Require(std::filesystem::exists(htmlPath), "HTML inspection report should be generated");

    const auto readFile = [](const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };

    const std::string json = readFile(jsonPath);
    const std::string html = readFile(htmlPath);
    apex::tests::Require(json.find("missing_mesh_count") != std::string::npos, "JSON report should include missing mesh count");
    apex::tests::Require(json.find("support_pkg") != std::string::npos, "JSON report should include package dependency names");
    apex::tests::Require(html.find("Robot Description Inspection") != std::string::npos, "HTML report should include the page title");
    apex::tests::Require(html.find("Missing Mesh URIs") != std::string::npos, "HTML report should include the missing mesh section");
}
