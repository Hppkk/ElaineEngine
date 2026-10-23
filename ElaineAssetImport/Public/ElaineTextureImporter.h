#pragma once
#include "ElaineResourceImporter.h"

namespace Elaine
{
    struct TextureImportSettings : ResourceImportSettings
    {
        std::string Dimension = "2d", Usage = "color";
        bool SRGB = false;
        TextureImportSettings() = default;
        TextureImportSettings(std::string Dimension, std::string Usage, bool SRGB)
            : Dimension(std::move(Dimension)), Usage(std::move(Usage)), SRGB(SRGB) {}
        std::shared_ptr<const ResourceImportSettings> Clone() const override { return std::make_shared<TextureImportSettings>(*this); }
        JsonCpp ToJson() const override { return {{"dimension", Dimension}, {"usage", Usage}, {"srgb", SRGB}}; }
    };

    struct TextureImportSource
    {
        std::string Name;
        std::vector<uint8_t> Bytes;
    };
    struct ImportedImage
    {
        int Width = 0, Height = 0;
        bool HDR = false;
        std::vector<uint8_t> Pixels;
    };
    class TextureImporter final : public ResourceImporter
    {
    public:
        const char* GetName() const override { return "Texture"; }
        std::vector<std::string> GetSupportedExtensions() const override;
        JsonCpp GetSettingsDescription() const override;
        std::shared_ptr<const ResourceImportSettings> ReadSettings(const JsonCpp&) const override;
        ResourceImportOutput Build(const ResourceImportContext&) const override;
        static ImportedAsset BuildTexture(ImportedAsset Asset, const TextureImportSettings&, const std::vector<TextureImportSource>&);
        static ImportedImage Decode(const std::vector<uint8_t>&);
        static std::vector<uint8_t> EncodePNG(int Width, int Height, const uint8_t* Pixels);
    };
}
