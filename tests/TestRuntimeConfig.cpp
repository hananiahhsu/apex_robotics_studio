#include "TestFramework.h"
#include "apex/platform/RuntimeConfig.h"

#include <filesystem>

ARS_TEST(TestRuntimeConfigRoundTripDefaultFile)
{
    const std::filesystem::path tempPath = std::filesystem::temp_directory_path() / "apex_runtime_test.ini";
    const apex::platform::RuntimeConfigLoader loader;
    loader.SaveDefaultFile(tempPath);
    const auto config = loader.LoadFromFile(tempPath);

    apex::tests::Require(config.EnableAuditPlugins, "Default config should enable audit plugins");
    apex::tests::RequireNear(config.PreferredJointVelocityLimitRadPerSec, 1.5, 1e-9, "Default preferred joint velocity limit should match");

    std::error_code errorCode;
    std::filesystem::remove(tempPath, errorCode);
}
