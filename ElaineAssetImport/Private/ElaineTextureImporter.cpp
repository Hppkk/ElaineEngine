#include "ElainePrecompiledHeader.h"
#include "ElaineTextureImporter.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#define TINYEXR_USE_MINIZ 0
#define TINYEXR_USE_STB_ZLIB 1
#define TINYEXR_IMPLEMENTATION
#include "tinyexr/tinyexr.h"
#include <fstream>
#include <bit>
#include <limits>

namespace Elaine
{
    namespace
    {
        std::vector<uint8_t> ReadSource(const std::filesystem::path& Path)
        {
            std::ifstream File(Path, std::ios::binary | std::ios::ate);
            if (!File) throw std::runtime_error("Cannot read texture source: " + Path.string());
            auto Length = File.tellg();
            if (Length <= 0 || uint64_t(Length) > uint64_t(INT_MAX)) throw std::runtime_error("Invalid texture source size");
            std::vector<uint8_t> Bytes(static_cast<size_t>(Length)); File.seekg(0);
            if (!File.read(reinterpret_cast<char*>(Bytes.data()), Length)) throw std::runtime_error("Truncated texture source");
            return Bytes;
        }
        std::vector<uint8_t> EncodeJson(const JsonCpp& Json) { const auto Text = Json.dump(); return {Text.begin(), Text.end()}; }
    }
    ImportedImage TextureImporter::Decode(const std::vector<uint8_t>& Bytes)
        {
            if (Bytes.empty() || Bytes.size() > INT_MAX) throw std::runtime_error("Invalid image size");
            ImportedImage Image; int Channels = 0;
            const bool EXR = Bytes.size() >= 4 && Bytes[0] == 0x76 && Bytes[1] == 0x2f && Bytes[2] == 0x31 && Bytes[3] == 1;
            Image.HDR = EXR || stbi_is_hdr_from_memory(Bytes.data(), static_cast<int>(Bytes.size()));
            void* Data = nullptr;
            if (EXR)
            {
                const char* Error = nullptr; float* Floats = nullptr;
                const int Result = LoadEXRFromMemory(&Floats, &Image.Width, &Image.Height, Bytes.data(), Bytes.size(), &Error);
                if (Result != TINYEXR_SUCCESS)
                { const std::string Message = Error ? Error : "Unsupported EXR"; if (Error) FreeEXRErrorMessage(Error); throw std::runtime_error(Message); }
                Data = Floats;
            }
            else if (Image.HDR) Data = stbi_loadf_from_memory(Bytes.data(), static_cast<int>(Bytes.size()), &Image.Width, &Image.Height, &Channels, 4);
            else Data = stbi_load_from_memory(Bytes.data(), static_cast<int>(Bytes.size()), &Image.Width, &Image.Height, &Channels, 4);
            auto Release = [&] { if (EXR) free(Data); else stbi_image_free(Data); };
            if (!Data) throw std::runtime_error(std::string("Image decode failed: ") + (stbi_failure_reason() ? stbi_failure_reason() : "unknown"));
            const uint64_t Size = uint64_t(Image.Width) * uint64_t(Image.Height) * (Image.HDR ? 16 : 4);
            if (Image.Width <= 0 || Image.Height <= 0 || Size > UINT32_MAX) { Release(); throw std::runtime_error("Texture dimensions exceed upload limits"); }
            Image.Pixels.resize(static_cast<size_t>(Size)); memcpy(Image.Pixels.data(), Data, Image.Pixels.size()); Release();
            // The container uses little-endian IEEE-754 float components.
            static_assert(std::endian::native == std::endian::little);
            return Image;
        }
    ImportedAsset TextureImporter::BuildTexture(ImportedAsset Asset,
        const TextureImportSettings& Settings, const std::vector<TextureImportSource>& Sources)
    {
        auto& Descriptor = Asset.Descriptor;
        const bool Cube = Settings.Dimension == "cube", Array = Settings.Dimension == "2darray";
        if (Settings.Dimension != "2d" && !Cube && !Array) throw std::runtime_error("Unsupported imported texture dimension");
        if (Sources.empty() || (Cube && Sources.size() != 6) || (!Cube && !Array && Sources.size() != 1)) throw std::runtime_error("Incorrect number of texture sources");
        if (Settings.Usage != "color" && Settings.Usage != "normal" && Settings.Usage != "data" && Settings.Usage != "hdr") throw std::runtime_error("Unsupported texture usage");
        AssetFile::Sections Sections;
        JsonCpp Texture = {{"version", 1}, {"dimension", Settings.Dimension}, {"usage", Settings.Usage}, {"srgb", Settings.SRGB},
            {"mips", 1}, {"layers", Array ? Sources.size() : size_t(1)}, {"faces", Cube ? 6 : 1}, {"subresources", JsonCpp::array()}};
        JsonCpp Import = {{"settings", {{"dimension", Settings.Dimension}, {"usage", Settings.Usage}, {"srgb", Settings.SRGB}}}, {"sources", JsonCpp::array()}};
        int Width = 0, Height = 0; bool HDR = false;
        for (size_t i = 0; i < Sources.size(); ++i)
        {
            const auto Image = Decode(Sources[i].Bytes);
            if (!i) { Width = Image.Width; Height = Image.Height; HDR = Image.HDR; }
            if (Image.Width != Width || Image.Height != Height || Image.HDR != HDR || (Cube && Width != Height)) throw std::runtime_error("Texture faces/layers must have matching dimensions and format");
            if (HDR && Settings.SRGB) throw std::runtime_error("HDR textures must use linear color space");
            auto& Pixels = Sections["data"];
            Texture["subresources"].push_back({{"mip", 0}, {"layer", Array ? i : size_t(0)}, {"face", Cube ? i : size_t(0)},
                {"width", Width}, {"height", Height}, {"offset", Pixels.size()}, {"size", Image.Pixels.size()}, {"rowPitch", size_t(Width) * (HDR ? 16 : 4)}});
            if (Image.Pixels.size() > UINT32_MAX - Pixels.size()) throw std::runtime_error("Texture exceeds upload limits");
            Pixels.insert(Pixels.end(), Image.Pixels.begin(), Image.Pixels.end());
            const std::string Section = "source" + std::to_string(i);
            Sections[Section] = Sources[i].Bytes;
            Import["sources"].push_back({{"name", Sources[i].Name}, {"section", Section}});
        }
        Texture["width"] = Width; Texture["height"] = Height; Texture["format"] = HDR ? "RGBA32F" : "RGBA8";
        Sections["texture"] = EncodeJson(Texture); Sections["import"] = EncodeJson(Import);
        Descriptor.Type = "Texture"; Descriptor.SourceFiles.clear(); Descriptor.Dependencies.clear();
        Descriptor.Tags = {{"Width", Width}, {"Height", Height}, {"Dimension", Settings.Dimension}, {"Format", Texture["format"]},
            {"Layers", Texture["layers"]}, {"Faces", Texture["faces"]}, {"Mips", 1}, {"SRGB", Settings.SRGB}, {"Usage", Settings.Usage}};
        Asset.Sections = std::move(Sections); return Asset;
    }

    std::vector<std::string> TextureImporter::GetSupportedExtensions() const
    { return {".png", ".jpg", ".jpeg", ".bmp", ".tga", ".ppm", ".hdr", ".exr"}; }
    JsonCpp TextureImporter::GetSettingsDescription() const
    { return {{"dimension", {"2d", "cube", "2darray"}}, {"usage", {"color", "normal", "data", "hdr"}}, {"srgb", "bool"}}; }
    std::shared_ptr<const ResourceImportSettings> TextureImporter::ReadSettings(const JsonCpp& Json) const
    { return std::make_shared<TextureImportSettings>(Json.value("dimension", "2d"), Json.value("usage", "color"), Json.value("srgb", false)); }
    ResourceImportOutput TextureImporter::Build(const ResourceImportContext& Context) const
    {
        const auto* Settings = dynamic_cast<const TextureImportSettings*>(Context.Request.Settings.get());
        if (!Settings) throw std::runtime_error("Texture importer requires TextureImportSettings");
        auto Asset = Context.MakeAsset("primary", "Texture", Context.Request.Destination);
        std::vector<TextureImportSource> Sources;
        if (!Context.Request.Sources.empty())
            for (const auto& File : Context.Request.Sources) Sources.push_back({File.filename().string(), ReadSource(File)});
        else if (Context.Request.Reimport)
        {
            const auto File = Context.Root / Context.Request.Destination;
            auto Bytes = AssetFile::ReadSection(File, "import"); auto Json = JsonCpp::parse(Bytes.begin(), Bytes.end());
            for (const auto& Source : Json.at("sources")) Sources.push_back({Source.at("name"), AssetFile::ReadSection(File, Source.at("section"))});
        }
        return {{BuildTexture(std::move(Asset), *Settings, Sources)}, {}};
    }
    std::vector<uint8_t> TextureImporter::EncodePNG(int Width, int Height, const uint8_t* Pixels)
    {
        std::vector<uint8_t> Bytes;
        auto Write = [](void* Context, void* Data, int Size) {
            auto& Out = *static_cast<std::vector<uint8_t>*>(Context);
            auto* Start = static_cast<uint8_t*>(Data); Out.insert(Out.end(), Start, Start + Size);
        };
        if (Width <= 0 || Height <= 0 || Width > INT_MAX / 4 || !Pixels ||
            !stbi_write_png_to_func(Write, &Bytes, Width, Height, 4, Pixels, Width * 4))
            throw std::runtime_error("Cannot encode embedded pixels");
        return Bytes;
    }
}
