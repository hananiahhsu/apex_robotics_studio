#include "TestFramework.h"
#include "apex/bundle/ProjectBundle.h"

#include <filesystem>
#include <fstream>

ARS_TEST(TestProjectBundleCreateAndLoad)
{
    const std::filesystem::path tempRoot = std::filesystem::temp_directory_path() / "apex_bundle_test";
    std::error_code errorCode;
    std::filesystem::remove_all(tempRoot, errorCode);
    std::filesystem::create_directories(tempRoot);

    const auto projectPath = tempRoot / "input.arsproject";
    const auto runtimePath = tempRoot / "runtime.ini";
    {
        std::ofstream project(projectPath);
        project << "[project]\n"
                << "schema_version=1\nproject_name=BundleTest\nrobot_name=Robot\n\n"
                << "[joint]\nname=J1\nlink_length=0.5\nmin_angle_deg=-180\nmax_angle_deg=180\n\n"
                << "[motion]\nstart_angles_deg=0\ngoal_angles_deg=5\nsample_count=2\nduration_seconds=1\nlink_safety_radius=0.05\n";
        std::ofstream runtime(runtimePath);
        runtime << "schema_version=1\n";
    }

    const auto pluginDir = tempRoot / "plugins_in";
    std::filesystem::create_directories(pluginDir);
    {
        std::ofstream manifest(pluginDir / "plugin.arsplugin");
        manifest << "[plugin]\nname=Dummy\nexecutable=dummy.exe\narguments=audit {project} {output}\n";
    }

    const auto bundleDir = tempRoot / "bundle";
    const apex::bundle::ProjectBundleManager manager;
    manager.CreateBundle(projectPath, runtimePath, bundleDir, &pluginDir);
    const auto manifest = manager.LoadManifest(bundleDir);

    apex::tests::Require(std::filesystem::exists(bundleDir / "bundle.arsbundle"), "Bundle manifest should exist");
    apex::tests::Require(std::filesystem::exists(manager.ResolveProjectPath(manifest, bundleDir)), "Bundled project should exist");
    apex::tests::Require(std::filesystem::exists(manager.ResolveRuntimePath(manifest, bundleDir)), "Bundled runtime should exist");
    apex::tests::Require(std::filesystem::exists(manager.ResolvePluginDirectory(manifest, bundleDir) / "plugin.arsplugin"), "Bundled plugin manifest should exist");

    std::filesystem::remove_all(tempRoot, errorCode);
}
