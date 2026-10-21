#include "ElainePrecompiledHeader.h"
#include "ElaineTextureUtils.h"
#include "ElaineTextureResource.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace Elaine
{
	void TextureUtils::Initialize()
	{
		//stbi_set_flip_vertically_on_load(true);
	}

	bool TextureUtils::LoadTextureFromFile(const std::string& InPath, TextureData& OutTexture)
	{
		//todo 
		int texChannels = 0;
		stbi_uc* PImageContent = stbi_load(InPath.c_str(), &OutTexture.mWidth, &OutTexture.mHeight, &texChannels, STBI_rgb_alpha);
		if (!PImageContent)
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

    bool TextureUtils::LoadTextureArrayFromFile(const std::vector<std::string>& Paths, TextureData& Out)
    {
        if (Paths.empty()) return false;
        std::vector<uint8_t> Pixels;
        for (size_t Layer = 0; Layer < Paths.size(); ++Layer)
        {
            int Width = 0, Height = 0, Channels = 0;
            auto* Image = stbi_load((Root::instance()->GetResourcePath() + Paths[Layer]).c_str(), &Width, &Height, &Channels, STBI_rgb_alpha);
            if (!Image) return false;
            if (Layer == 0) { Out.mWidth = Width; Out.mHeight = Height; }
            if (Width != Out.mWidth || Height != Out.mHeight) { stbi_image_free(Image); return false; }
            Pixels.insert(Pixels.end(), Image, Image + size_t(Width) * Height * 4);
            stbi_image_free(Image);
        }
        Out.mDepth = static_cast<int>(Paths.size());
        Out.mContentSize = Pixels.size();
        Out.mContent = static_cast<uint8*>(Memory::SystemMalloc(Pixels.size()));
        Memory::MemoryCopy(Out.mContent, Pixels.data(), Pixels.size());
        Out.mFormat = PF_R8G8B8A8;
        return true;
    }


	bool TextureUtils::LoadTextureFromMemory(const char* InContents, TextureData& OutTexture)
	{
		return true;
	}
}