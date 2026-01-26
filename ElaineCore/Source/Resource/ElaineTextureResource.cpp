#include "ElainePrecompiledHeader.h"
#include "ElaineTextureResource.h"
#include "ElaineTextureUtils.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineDataStream.h"

namespace Elaine
{
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
		SAFE_DELETE(mTextureRHI);
	}

	bool Texture::LoadImpl()
	{
		DataStream Stream(Root::instance()->GetResourcePath() + mResourceName);
		Stream.ReadAll();

		JsonCpp JsonData = JsonCpp::parse(Stream.GetDataStream());

		// dimension
		mVersion = Version(JsonData["version"]);
		std::string dimension = JsonData["dimension"];
		if (dimension == "2d")
			mTextureDesc.mDimension = TextureDimension::Texture2D;
		else if (dimension == "cube")
			mTextureDesc.mDimension = TextureDimension::TextureCube;
		else if (dimension == "2darray")
			mTextureDesc.mDimension = TextureDimension::Texture2DArray;
		else if (dimension == "cubearray")
			mTextureDesc.mDimension = TextureDimension::TextureCubeArray;
		else if (dimension == "3d")
			mTextureDesc.mDimension = TextureDimension::Texture3D;

		// usage
		std::string usage = JsonData.value("usage", "color");
		if (usage == "color")   
			mTextureDesc.mUsage = TextureUsage::Color;
		else if (usage == "normal")  
			mTextureDesc.mUsage = TextureUsage::Normal;
		else if (usage == "hdr")     // HDR ¡ú Cubemap
			mTextureDesc.mUsage = TextureUsage::HDR;

		//num mips
		mTextureDesc.mNumMips = JsonData.value("nummips", 1);

		mTextureDesc.mbSRGB = JsonData.value("srgb", false);

		// source
		const JsonCpp& source = JsonData["source"];

		switch (mTextureDesc.mDimension)
		{
		case TextureDimension::Texture2D:
			mTexturePath.push_back(source["path"]);
			break;
		case TextureDimension::TextureCube:
		{
			//TEXTURE_CUBE_MAP_POSITIVE_X, //right
			//TEXTURE_CUBE_MAP_NEGATIVE_X, //left
			//TEXTURE_CUBE_MAP_POSITIVE_Y, //top
			//TEXTURE_CUBE_MAP_NEGATIVE_Y, //bottom
			//TEXTURE_CUBE_MAP_POSITIVE_Z, //front
			//TEXTURE_CUBE_MAP_NEGATIVE_Z, //back
			mTexturePath.reserve(6);
			static const char* Faces[6] = { "px","nx","py","ny","pz","nz" };
			for (auto face : Faces)
			{
				mTexturePath.push_back(source[face]);
			}
			break;
		}
		case TextureDimension::Texture2DArray:
			break;
		default:
			break;
		}

		bool LoadSucceed = false;

		RHITextureDesc TexDescRHI;

		TexDescRHI.mNumMips = mTextureDesc.mNumMips;
		TexDescRHI.mNumSamples = 1;
		TexDescRHI.mFlags = TextureCreateFlags::ShaderResource;
		TexDescRHI.mDimension = mTextureDesc.mDimension;

		if (mTextureDesc.mDimension == TextureDimension::Texture2D)
		{
			LoadSucceed = TextureUtils::LoadTextureFromFile(mResourceName, mTextureData);
			TexDescRHI.mArraySize = 1;
		}
		else if (mTextureDesc.mDimension == TextureDimension::Texture2DArray || mTextureDesc.mDimension == TextureDimension::TextureCube)
		{
			LoadSucceed = TextureUtils::LoadTextureArrayFromFile(mTexturePath, mTextureData);
			TexDescRHI.mArraySize = 6;
		}
		TexDescRHI.mExtent.x = mTextureData.mWidth;
		TexDescRHI.mExtent.y = mTextureData.mHeight;
		TexDescRHI.mFormat = mTextureData.mFormat;

		if (!LoadSucceed)
		{
			LOG_ERROR("Failed to load texture.");
			return false;
		}

		mTextureRHI = GetDynamicRHI()->GetDefaultCommandContext()->RHICreateTexture(TexDescRHI, mTextureData.mContent);
		return true;
	}

	void Texture::UnloadImpl()
	{

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

	void Texture::ResourceArrivedImpl()
	{
	}

	const std::string& Texture::GetCubeMapPath(TextureCubeMapFace InFace)
	{
		if (InFace >= mTexturePath.size())
			return StringUtils::EMPTY;

		return mTexturePath[InFace];
	}
}