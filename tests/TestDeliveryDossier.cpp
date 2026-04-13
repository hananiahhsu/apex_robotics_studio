#include "TestFramework.h"

#include "apex/delivery/DeliveryDossier.h"

#include <filesystem>
#include <fstream>

ARS_TEST(TestDeliveryDossierBuilderWritesArtifacts)
{
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "apex_delivery_dossier_test";
    std::filesystem::create_directories(directory);

    apex::delivery::DeliveryDossierManifest manifest;
    manifest.Title = "Delivery Dossier";
    manifest.ProjectName = "ProjectX";
    manifest.GateStatus = apex::quality::GateStatus::Pass;
    manifest.SessionStatus = apex::session::SessionStatus::Success;
    manifest.Artifacts.push_back({"report", "reports/report.html", "Main report"});

    const apex::delivery::DeliveryDossierBuilder builder;
    const auto jsonPath = directory / "manifest.json";
    const auto htmlPath = directory / "index.html";
    builder.SaveManifestJson(manifest, jsonPath);
    builder.SaveIndexHtml(manifest, htmlPath);

    apex::tests::Require(std::filesystem::exists(jsonPath), "Manifest JSON should be written.");
    apex::tests::Require(std::filesystem::exists(htmlPath), "Index HTML should be written.");

    std::ifstream stream(htmlPath);
    std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    apex::tests::Require(content.find("reports/report.html") != std::string::npos, "Index should reference artifact.");

    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
}
