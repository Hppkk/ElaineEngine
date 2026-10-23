#pragma once
#include "ElaineResourceImporter.h"

namespace Elaine
{
    struct MeshImportSettings : ResourceImportSettings
    {
        float Scale = 1.0f;
        std::string Material = "material/ImportedPBR.asset";
        std::shared_ptr<const ResourceImportSettings> Clone() const override { return std::make_shared<MeshImportSettings>(*this); }
        JsonCpp ToJson() const override { return {{"scale", Scale}, {"material", Material}}; }
    };
    class MeshImporter final : public ResourceImporter
    {
    public:
        const char* GetName() const override { return "Mesh"; }
        std::vector<std::string> GetSupportedExtensions() const override { return {".fbx", ".obj", ".gltf", ".glb"}; }
        JsonCpp GetSettingsDescription() const override;
        std::shared_ptr<const ResourceImportSettings> ReadSettings(const JsonCpp&) const override;
        ResourceImportOutput Build(const ResourceImportContext&) const override;
    };
    // Offline only: add metadata without changing the existing geometry bytes or ID.
    bool UpgradeMeshAsset(const std::filesystem::path& File);
    JsonCpp BuildMeshAssetMetadata(const std::vector<uint8_t>& Geometry);
}
