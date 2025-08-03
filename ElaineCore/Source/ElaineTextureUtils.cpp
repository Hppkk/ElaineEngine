#include "ElainePrecompiledHeader.h"
#include "ElaineTextureUtils.h"
#include "ElaineTextureResource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Elaine
{
	void TextureUtils::Initilize()
	{
		//stbi_set_flip_vertically_on_load(true);
	}
	bool TextureUtils::LoadTextureFromFile(const std::string& InPath, TextureData& OutTexture)
	{
		//todo 
		int texChannels = 0;
		stbi_uc* PImageContent = stbi_load(InPath.c_str(), &OutTexture.mWidth, &OutTexture.mHeight, &texChannels, STBI_rgb_alpha);
		if (!OutTexture.mContent)
		{
			LOG_INFO("Load texture failed.");
			return false;
		}
		OutTexture.mDepth = 1;
		OutTexture.mContentSize = OutTexture.mWidth * OutTexture.mHeight * sizeof(unsigned char) * 4;
		OutTexture.mFormat = PF_R8G8B8A8;
		OutTexture.mContent = (uint8*)Memory::SystemMalloc(OutTexture.mContentSize);
		Memory::MemoryCopy(OutTexture.mContent, PImageContent, OutTexture.mContentSize);
		stbi_image_free(PImageContent);
		return true;
	}

	bool TextureUtils::LoadTexture3DFromFile(Texture* InTexResource, TextureData& OutTexture)
	{
		size_t LayerSize = 0u;

		for (int Index = TEXTURE_CUBE_MAP_POSITIVE_X; Index < TEXTURE_CUBE_MAP_MAX; ++Index)
		{
			//todo 
			int texChannels = 0;
			stbi_uc* PImageContent = stbi_load((Root::instance()->getResourcePath() + InTexResource->GetCubeMapPath((TextureCubeMapFace)Index)).c_str(), &OutTexture.mWidth, &OutTexture.mHeight, &texChannels, STBI_rgb_alpha);
			if (!PImageContent)
			{
				LOG_INFO("Load texture failed.");
				return false;
			}
			if (OutTexture.mContent == nullptr)
			{
				LayerSize = OutTexture.mWidth * OutTexture.mHeight * sizeof(unsigned char) * 4;
				OutTexture.mDepth = 6;
				OutTexture.mContentSize = LayerSize * OutTexture.mDepth;
				OutTexture.mContent = (uint8*)Memory::SystemMalloc(OutTexture.mContentSize);
			}
			
			Memory::MemoryCopy(OutTexture.mContent + LayerSize * Index, PImageContent, LayerSize);
			stbi_image_free(PImageContent);
		}
		
		
		OutTexture.mFormat = PF_R8G8B8A8;
		return true;
	}


	bool TextureUtils::LoadTextureFromMemory(const char* InContents, TextureData& OutTexture)
	{
		return true;
	}
}