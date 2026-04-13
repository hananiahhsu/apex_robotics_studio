#include "TestFramework.h"

#include "apex/sweep/ParameterSweep.h"

#include <filesystem>

ARS_TEST(TestParameterSweepRoundTrip)
{
    const auto filePath = std::filesystem::temp_directory_path() / "apex_sweep_test_v11.arssweep";
    apex::sweep::SweepDefinition sweep;
    sweep.Name = "Sweep";
    sweep.BaseProjectPath = "demo.arsproject";
    sweep.RuntimePath = "runtime.ini";
    sweep.SampleCounts = {12, 18};
    sweep.DurationScales = {0.9, 1.1};
    sweep.SafetyRadii = {0.06, 0.08};

    apex::sweep::ParameterSweepSerializer().SaveToFile(sweep, filePath);
    const auto loaded = apex::sweep::ParameterSweepSerializer().LoadFromFile(filePath);

    apex::tests::Require(loaded.SampleCounts.size() == 2, "Sweep sample counts should round trip.");
    apex::tests::RequireNear(loaded.DurationScales[1], 1.1, 1e-9, "Duration scale should round trip.");
    apex::tests::RequireNear(loaded.SafetyRadii[0], 0.06, 1e-9, "Safety radius should round trip.");

    std::error_code ec;
    std::filesystem::remove(filePath, ec);
}
