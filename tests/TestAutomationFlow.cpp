#include "TestFramework.h"

#include "apex/workflow/AutomationFlow.h"

#include <filesystem>

ARS_TEST(TestAutomationFlowRoundTrip)
{
    const auto filePath = std::filesystem::temp_directory_path() / "apex_flow_test_v11.arsflow";
    apex::workflow::AutomationFlow flow;
    flow.Name = "Demo Flow";
    flow.BaseProjectPath = "demo.arsproject";
    flow.RuntimePath = "runtime.ini";
    flow.OutputDirectory = "workspace";
    flow.Steps.push_back({"Gate", "gate_project", "reports/gate.html"});
    flow.Steps.push_back({"Delivery", "create_delivery_dossier", "delivery"});

    apex::workflow::AutomationFlowSerializer().SaveToFile(flow, filePath);
    const auto loaded = apex::workflow::AutomationFlowSerializer().LoadFromFile(filePath);

    apex::tests::Require(loaded.Name == flow.Name, "Flow name should round trip.");
    apex::tests::Require(loaded.Steps.size() == 2, "Flow steps should round trip.");
    apex::tests::Require(loaded.Steps[1].Action == "create_delivery_dossier", "Flow action should survive round trip.");

    std::error_code ec;
    std::filesystem::remove(filePath, ec);
}
