#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	enum TextureCubeMapFace
	{
		TEXTURE_CUBE_MAP_POSITIVE_X, //right
		TEXTURE_CUBE_MAP_NEGATIVE_X, //left
		TEXTURE_CUBE_MAP_POSITIVE_Y, //top
		TEXTURE_CUBE_MAP_NEGATIVE_Y, //bottom
		TEXTURE_CUBE_MAP_POSITIVE_Z, //front
		TEXTURE_CUBE_MAP_NEGATIVE_Z, //back
		TEXTURE_CUBE_MAP_MAX,
	};

	struct TextureData
	{
		int32 mWidth = 0;
		int32 mHeight = 0;
		int32 mDepth = 0;
		uint8* mContent = nullptr;
		size_t mContentSize = 0;
		PixelFormat mFormat = PF_Unknown;
	}; 

	enum class TextureUsage
	{
		Color,
		Normal,
		Data,
		HDR,
		RenderTarget
	};

	struct TextureDesc
	{
		int mNumMips = 1;
		TextureDimension mDimension = TextureDimension::Texture2D;
		TextureUsage mUsage = TextureUsage::Color;
		PixelFormat mFormat = PF_Unknown;
		uint32 mWidth = 0;
		uint32 mHeight = 0;
		uint32 mDepth = 1;

		bool mbSRGB = false;
	};
}