#pragma once

#include "apex/quality/ReleaseGate.h"
#include "apex/session/ExecutionSession.h"

#include <filesystem>
#include <string>
#include <vector>

namespace apex::delivery
{
    struct DeliveryArtifact
    {
        std::string Type;
        std::filesystem::path RelativePath;
        std::string Description;
    };

    struct DeliveryDossierManifest
    {
        std::string Title;
        std::string ProjectName;
        apex::quality::GateStatus GateStatus = apex::quality::GateStatus::Pass;
        apex::session::SessionStatus SessionStatus = apex::session::SessionStatus::Success;
        std::vector<DeliveryArtifact> Artifacts;
    };

    class DeliveryDossierBuilder
    {
    public:
        void SaveManifestJson(const DeliveryDossierManifest& manifest, const std::filesystem::path& filePath) const;
        [[nodiscard]] std::string BuildIndexHtml(const DeliveryDossierManifest& manifest) const;
        void SaveIndexHtml(const DeliveryDossierManifest& manifest, const std::filesystem::path& filePath) const;
    };
}
