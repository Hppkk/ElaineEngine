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
}