#include "ElainePrecompiledHeader.h"
#include "ElaineTextureResource.h"
#include "ElaineTextureUtils.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	Texture::Texture(ResourceManager* InManager, TextureDimension InDimension, const std::string& InResName, const std::vector<std::string> InCubePaths)
		: ResourceBase(InManager, InResName)
		, mDimension(InDimension)
	{
		if (mDimension == TextureDimension::Texture2DArray || mDimension == TextureDimension::TextureCube)
		{
			assert(InCubePaths.size() == 6);
			for (int Index = TEXTURE_CUBE_MAP_POSITIVE_X; Index < TEXTURE_CUBE_MAP_MAX; ++Index)
			{
				mCubePaths[Index] = InCubePaths[Index];
			}
		}
	}

	Texture::~Texture()
	{
		if (mTextureData.mContent)
		{
			Memory::SystemFree(mTextureData.mContent);
		}
		SAFE_DELETE(mTextureRHI);
	}

	void Texture::loadImpl()
	{
		bool LoadSucceed = false;

		RHITextureDesc TexDescRHI;

		TexDescRHI.mNumMips = 1;
		TexDescRHI.mNumSamples = 1;
		TexDescRHI.mFlags = TextureCreateFlags::ShaderResource;
		TexDescRHI.mDimension = mDimension;

		if (mDimension == TextureDimension::Texture2D)
		{
			LoadSucceed = TextureUtils::LoadTextureFromFile(msResName, mTextureData);
			TexDescRHI.mArraySize = 1;
		}
		else if (mDimension == TextureDimension::Texture2DArray || mDimension == TextureDimension::TextureCube)
		{
			LoadSucceed = TextureUtils::LoadTexture3DFromFile(this, mTextureData);
			TexDescRHI.mArraySize = 6;
		}
		TexDescRHI.mExtent.x = mTextureData.mWidth;
		TexDescRHI.mExtent.y = mTextureData.mHeight;
		TexDescRHI.mFormat = mTextureData.mFormat;
		mTextureRHI = GetDynamicRHI()->GetDefaultCommandContext()->RHICreateTexture(TexDescRHI, mTextureData.mContent);
	}

	void Texture::unloadImpl()
	{

	}

	const std::string& Texture::GetCubeMapPath(TextureCubeMapFace InFace)
	{
		return mCubePaths[InFace];
	}
}