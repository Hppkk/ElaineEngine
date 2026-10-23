#include "ElainePrecompiledHeader.h"
#include "ElaineTextureResource.h"
#include "Resource/ElaineAsset.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
    struct Texture::LoadData : ResourceLoadData
    {
        TextureDesc Description;
        TextureData Pixels;
        RHITextureHandle Texture;
        RHITextureDesc GPUDescription;
        ~LoadData() override { if (Pixels.mContent) Memory::SystemFree(Pixels.mContent); }
    };
    Texture::Texture(ResourceManager* Manager, const std::string& Path) : ResourceBase(Manager, Path) {}
    Texture::~Texture() { if (mTextureData.mContent) Memory::SystemFree(mTextureData.mContent); }
    std::unique_ptr<ResourceLoadData> Texture::LoadImpl()
    {
        auto Pending = std::make_unique<LoadData>(); auto& Data = *Pending;
        const auto File = std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName;
        auto LayoutBytes = AssetFile::ReadSection(File, "texture");
        const auto Layout = JsonCpp::parse(LayoutBytes.begin(), LayoutBytes.end());
        const auto Dimension = Layout.at("dimension").get<std::string>();
        const uint64_t Width = Layout.at("width").get<uint64_t>(), Height = Layout.at("height").get<uint64_t>();
        const uint64_t Layers = Layout.at("layers").get<uint64_t>(), Faces = Layout.at("faces").get<uint64_t>();
        const auto Format = Layout.at("format").get<std::string>();
        if (Layout.at("version") != 1 || Layout.at("mips") != 1 || !Width || !Height || Width > INT32_MAX || Height > INT32_MAX ||
            !Layers || Layers > UINT32_MAX || (Format != "RGBA8" && Format != "RGBA32F")) throw std::runtime_error("Unsupported texture layout");
        if (Dimension == "2d" && Layers == 1 && Faces == 1) Data.Description.mDimension = TextureDimension::Texture2D;
        else if (Dimension == "cube" && Layers == 1 && Faces == 6 && Width == Height) Data.Description.mDimension = TextureDimension::TextureCube;
        else if (Dimension == "2darray" && Faces == 1) Data.Description.mDimension = TextureDimension::Texture2DArray;
        else throw std::runtime_error("Unsupported texture dimension or layer count");
        const uint64_t PixelBytes = Format == "RGBA32F" ? 16 : 4;
        if (Width > UINT32_MAX / PixelBytes || Height > UINT32_MAX / (Width * PixelBytes)) throw std::runtime_error("Texture exceeds upload limits");
        const uint64_t LayerSize = Width * Height * PixelBytes;
        if (Faces * Layers > UINT32_MAX / LayerSize) throw std::runtime_error("Texture exceeds upload limits");
        auto Pixels = AssetFile::ReadSection(File, "data");
        if (Pixels.size() != LayerSize * Layers * Faces || !Layout.at("subresources").is_array() || Layout.at("subresources").size() != Layers * Faces)
            throw std::runtime_error("Texture payload size mismatch");
        for (size_t i = 0; i < Layout.at("subresources").size(); ++i)
        {
            const auto& Sub = Layout.at("subresources")[i];
            if (Sub.at("mip") != 0 || Sub.at("layer") != (Dimension == "2darray" ? i : size_t(0)) || Sub.at("face") != (Dimension == "cube" ? i : size_t(0)) ||
                Sub.at("width") != Width || Sub.at("height") != Height || Sub.at("offset") != i * LayerSize || Sub.at("size") != LayerSize || Sub.at("rowPitch") != Width * PixelBytes)
                throw std::runtime_error("Invalid texture subresource layout");
        }
        Data.Description.mWidth = static_cast<uint32_t>(Width); Data.Description.mHeight = static_cast<uint32_t>(Height);
        Data.Description.mDepth = 1; Data.Description.mNumMips = 1;
        Data.Description.mFormat = Format == "RGBA32F" ? PF_A32B32G32R32F : PF_R8G8B8A8;
        Data.Description.mbSRGB = Layout.at("srgb").get<bool>();
        if (Format == "RGBA32F" && Data.Description.mbSRGB) throw std::runtime_error("HDR texture cannot use sRGB");
        const auto Usage = Layout.at("usage").get<std::string>();
        if (Usage == "color") Data.Description.mUsage = TextureUsage::Color;
        else if (Usage == "normal") Data.Description.mUsage = TextureUsage::Normal;
        else if (Usage == "data") Data.Description.mUsage = TextureUsage::Data;
        else if (Usage == "hdr") Data.Description.mUsage = TextureUsage::HDR;
        else throw std::runtime_error("Unknown texture usage");
        Data.Pixels.mWidth = static_cast<int32_t>(Width); Data.Pixels.mHeight = static_cast<int32_t>(Height);
        Data.Pixels.mDepth = static_cast<int32_t>(Faces * Layers); Data.Pixels.mFormat = Data.Description.mFormat;
        Data.Pixels.mContentSize = Pixels.size(); Data.Pixels.mContent = static_cast<uint8_t*>(Memory::SystemMalloc(Pixels.size()));
        if (!Data.Pixels.mContent) throw std::bad_alloc();
        memcpy(Data.Pixels.mContent, Pixels.data(), Pixels.size());
        Data.GPUDescription.mExtent.x = static_cast<uint32_t>(Width); Data.GPUDescription.mExtent.y = static_cast<uint32_t>(Height);
        Data.GPUDescription.mDimension = Data.Description.mDimension; Data.GPUDescription.mArraySize = static_cast<uint32_t>(Layers);
        Data.GPUDescription.mFormat = Data.Description.mFormat; Data.GPUDescription.mNumMips = 1; Data.GPUDescription.mNumSamples = 1;
        Data.GPUDescription.mFlags = TextureCreateFlags::ShaderResource;
        if (Data.Description.mbSRGB) Data.GPUDescription.mFlags |= TextureCreateFlags::SRGB;
        Data.MemoryUsage = Pixels.size(); return Pending;
    }
    bool Texture::InitializeGPU(ResourceLoadData& InData)
    {
        auto& Data = static_cast<LoadData&>(InData); CheckRHICreationThread();
        auto* Context = GetDynamicRHI()->GetDefaultCommandContext();
        Data.Texture = Context->RHICreateTexture(Data.GPUDescription, nullptr);
        if (Data.Texture) Context->UploadTexture(Data.Texture.get(), Data.Pixels.mContent, Data.Pixels.mContentSize);
        return Data.Texture != nullptr;
    }
    void Texture::ApplyLoadedData(ResourceLoadData& InData) noexcept
    {
        auto& Data = static_cast<LoadData&>(InData);
        std::swap(mTextureDesc, Data.Description); std::swap(mTextureData, Data.Pixels);
        mTextureRHI.swap(Data.Texture); std::swap(mPendingRHIDesc, Data.GPUDescription);
    }
    void Texture::UnloadImpl()
    {
        mTextureRHI.reset(); if (mTextureData.mContent) Memory::SystemFree(mTextureData.mContent); mTextureData = {};
    }
    std::vector<std::string> Texture::GetSourceFiles() const { return {mResourceName}; }
    void Texture::SaveResourceImpl()
    {
        const auto File = std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName;
        const auto Metadata = AssetFile::ReadMetadata(File); AssetFile::Sections Sections;
        for (const auto& Entry : Metadata.at("sections").items()) if (Entry.key() != "data") Sections[Entry.key()] = AssetFile::ReadSection(File, Entry.key());
        if (!mTextureData.mContent || !mTextureData.mContentSize) throw std::runtime_error("Cannot save unloaded texture");
        Sections["data"] = {mTextureData.mContent, mTextureData.mContent + mTextureData.mContentSize};
        AssetFile::Write(File, GetDescriptor(), Sections);
    }
}
