#include "ElainePrecompiledHeader.h"
#include "ElaineTextureResource.h"
#include "ElaineTextureUtils.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineDataStream.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
    struct Texture::LoadData : ResourceLoadData
    {
        Version mVersion;
        TextureDesc mTextureDesc;
        TextureData mTextureData;
        std::vector<std::string> mTexturePath;
        RHITextureHandle mTextureRHI;
        RHITextureDesc mPendingRHIDesc;
        ~LoadData() override { if (mTextureData.mContent) Memory::SystemFree(mTextureData.mContent); }
    };


	static std::string DimensionToString(TextureDimension InTextureDimension)
	{
		switch (InTextureDimension)
		{
		case TextureDimension::Texture2D:        return "2d";
		case TextureDimension::TextureCube:      return "cube";
		case TextureDimension::Texture2DArray:   return "2darray";
		case TextureDimension::TextureCubeArray: return "cubearray";
		case TextureDimension::Texture3D:        return "3d";
		default:                                 return "unknown";
		}
	}

	static std::string UsageToString(TextureUsage InTextureUsage)
	{
		switch (InTextureUsage)
		{
		case TextureUsage::Color:   return "color";
		case TextureUsage::Normal:  return "normal";
		case TextureUsage::HDR:     return "hdr";
		default:                    return "color";
		}
	}

	Texture::Texture(ResourceManager* InManager, const std::string& InResName)
		: ResourceBase(InManager, InResName)
	{

	}

	Texture::~Texture()
	{
		if (mTextureData.mContent)
		{
			Memory::SystemFree(mTextureData.mContent);
		}
		mTextureRHI.reset();
	}

	std::unique_ptr<ResourceLoadData> Texture::LoadImpl()
    {
        auto Pending = std::make_unique<LoadData>();
        auto& Data = *Pending;
		DataStream Stream(Root::instance()->GetResourcePath() + mResourceName);
		Stream.ReadAll();

		JsonCpp JsonData = JsonCpp::parse(Stream.GetDataStream());

		// dimension
		Data.mVersion = Version(JsonData["version"]);
		std::string dimension = JsonData["dimension"];
		if (dimension == "2d")
			Data.mTextureDesc.mDimension = TextureDimension::Texture2D;
		else if (dimension == "cube")
			Data.mTextureDesc.mDimension = TextureDimension::TextureCube;
		else if (dimension == "2darray")
			Data.mTextureDesc.mDimension = TextureDimension::Texture2DArray;
		else if (dimension == "cubearray")
			Data.mTextureDesc.mDimension = TextureDimension::TextureCubeArray;
		else if (dimension == "3d")
			Data.mTextureDesc.mDimension = TextureDimension::Texture3D;
        else throw std::runtime_error("Unknown texture dimension");
        if (Data.mTextureDesc.mDimension == TextureDimension::Texture3D || Data.mTextureDesc.mDimension == TextureDimension::TextureCubeArray)
            throw std::runtime_error("Texture file decoder does not support 3D or cube arrays");

		// usage
		std::string usage = JsonData.value("usage", "color");
		if (usage == "color")   
			Data.mTextureDesc.mUsage = TextureUsage::Color;
		else if (usage == "normal")  
			Data.mTextureDesc.mUsage = TextureUsage::Normal;
		else if (usage == "hdr")     // HDR ¡ú Cubemap
			Data.mTextureDesc.mUsage = TextureUsage::HDR;

		//num mips
		Data.mTextureDesc.mNumMips = JsonData.value("nummips", 1);

		Data.mTextureDesc.mbSRGB = JsonData.value("srgb", false);

		// source
		const JsonCpp& source = JsonData["source"];

		switch (Data.mTextureDesc.mDimension)
		{
		case TextureDimension::Texture2D:
			Data.mTexturePath.push_back(source["path"]);
			break;
		case TextureDimension::TextureCube:
		{
			//TEXTURE_CUBE_MAP_POSITIVE_X, //right
			//TEXTURE_CUBE_MAP_NEGATIVE_X, //left
			//TEXTURE_CUBE_MAP_POSITIVE_Y, //top
			//TEXTURE_CUBE_MAP_NEGATIVE_Y, //bottom
			//TEXTURE_CUBE_MAP_POSITIVE_Z, //front
			//TEXTURE_CUBE_MAP_NEGATIVE_Z, //back
			Data.mTexturePath.reserve(6);
			static const char* Faces[6] = { "px","nx","py","ny","pz","nz" };
			for (auto face : Faces)
			{
				Data.mTexturePath.push_back(source[face]);
			}
			break;
		}
		case TextureDimension::Texture2DArray:
            for (const auto& Path : source.at("paths")) Data.mTexturePath.push_back(Path.get<std::string>());
            if (Data.mTexturePath.empty()) throw std::runtime_error("Texture array contains no layers");
			break;
		default:
			break;
		}

		bool LoadSucceed = false;

		RHITextureDesc TexDescRHI;

		TexDescRHI.mNumMips = Data.mTextureDesc.mNumMips;
		TexDescRHI.mNumSamples = 1;
		TexDescRHI.mFlags = TextureCreateFlags::ShaderResource;
		TexDescRHI.mDimension = Data.mTextureDesc.mDimension;
        if (Data.mTextureDesc.mbSRGB) TexDescRHI.mFlags |= TextureCreateFlags::SRGB;

		if (Data.mTextureDesc.mDimension == TextureDimension::Texture2D)
		{
			LoadSucceed = !Data.mTexturePath.empty() && TextureUtils::LoadTextureFromFile(Root::instance()->GetResourcePath() + Data.mTexturePath.front(), Data.mTextureData);
			TexDescRHI.mArraySize = 1;
		}
		else if (Data.mTextureDesc.mDimension == TextureDimension::Texture2DArray || Data.mTextureDesc.mDimension == TextureDimension::TextureCube)
		{
			LoadSucceed = TextureUtils::LoadTextureArrayFromFile(Data.mTexturePath, Data.mTextureData);
			TexDescRHI.mArraySize = Data.mTextureDesc.mDimension == TextureDimension::TextureCube ? 1 : static_cast<uint32>(Data.mTexturePath.size());
		}
		TexDescRHI.mExtent.x = Data.mTextureData.mWidth;
		TexDescRHI.mExtent.y = Data.mTextureData.mHeight;
		TexDescRHI.mFormat = Data.mTextureData.mFormat;

		if (!LoadSucceed)
		{
			LOG_ERROR("Failed to load texture.");
			return {};
		}

		if (TexDescRHI.mNumMips != 1) throw std::runtime_error("Texture decoder supplies one mip; nummips must be 1"); // Decoders currently supply one mip only.
        Data.mPendingRHIDesc = TexDescRHI;
		Data.MemoryUsage = Data.mTextureData.mContentSize;
        return Pending;
	}

    std::vector<std::string> Texture::GetSourceFiles() const
    {
        auto Files = mTexturePath; Files.push_back(mResourceName); return Files;
    }
    bool Texture::InitializeGPU(ResourceLoadData& InData)
    {
        auto& Data = static_cast<LoadData&>(InData);
        CheckRHICreationThread();
        auto* Context = GetDynamicRHI()->GetDefaultCommandContext();
        Data.mTextureRHI = Context->RHICreateTexture(Data.mPendingRHIDesc, nullptr);
        if (Data.mTextureRHI) Context->UploadTexture(Data.mTextureRHI.get(), Data.mTextureData.mContent, Data.mTextureData.mContentSize);
        return Data.mTextureRHI != nullptr;
    }

	void Texture::UnloadImpl()
    {
        mTextureRHI.reset();
        if (mTextureData.mContent) Memory::SystemFree(mTextureData.mContent);
        mTextureData = {};
        mTexturePath.clear();
    }

	void Texture::SaveResourceImpl()
	{
		JsonCpp JsonData;
		mVersion = CurrentEngineVersion;
		// version
		JsonData["version"] = mVersion.ToString();

		// dimension
		JsonData["dimension"] = DimensionToString(mTextureDesc.mDimension);

		// usage
		JsonData["usage"] = UsageToString(mTextureDesc.mUsage);

		// srgb
		JsonData["srgb"] = mTextureDesc.mbSRGB;

		// num mips
		JsonData["nummips"] = mTextureDesc.mNumMips;

		// source
		JsonCpp source;

		switch (mTextureDesc.mDimension)
		{
		case TextureDimension::Texture2D:
		{
			if (!mTexturePath.empty())
				source["path"] = mTexturePath[0];
			break;
		}

		case TextureDimension::TextureCube:
		{
			static const char* Faces[6] = { "px","nx","py","ny","pz","nz" };

			for (size_t i = 0; i < 6 && i < mTexturePath.size(); ++i)
			{
				source[Faces[i]] = mTexturePath[i];
			}
			break;
		}

		case TextureDimension::Texture2DArray:
		{
            source["paths"] = mTexturePath;
			break;
		}

		default:
			break;
		}

		JsonData["source"] = source;

		std::string JsonString = JsonData.dump(4);
		//.texture
		std::string FullPath = Root::instance()->GetResourcePath() + mResourceName;
		DataStream SaveDataStream(FullPath, DataStream::Out);
		SaveDataStream.Write(JsonString.c_str(), JsonString.size() + 1);
	}



	const std::string& Texture::GetCubeMapPath(TextureCubeMapFace InFace)
	{
        if (InFace >= mTexturePath.size())
			return StringUtils::EMPTY;

		return mTexturePath[InFace];
	}
    void Texture::ApplyLoadedData(ResourceLoadData& InData) noexcept
    {
        auto& Data = static_cast<LoadData&>(InData);
        std::swap(mVersion, Data.mVersion);
        std::swap(mTextureDesc, Data.mTextureDesc);
        std::swap(mTextureData, Data.mTextureData);
        std::swap(mTexturePath, Data.mTexturePath);
        std::swap(mTextureRHI, Data.mTextureRHI);
        std::swap(mPendingRHIDesc, Data.mPendingRHIDesc);
    }

}