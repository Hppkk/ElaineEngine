#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineTextureBase.h"
#include <string>

namespace Elaine
{
	class Texture;

	class ElaineCoreExport TextureUtils
	{
	public:
		static void Initialize();
		static bool LoadTextureFromFile(const std::string& InPath, TextureData& OutTexture);
		static bool LoadTextureArrayFromFile(const std::vector<std::string>& InPaths, TextureData& OutTexture);
		static bool LoadTextureFromMemory(const char* InContents, TextureData& OutTexture);
	};
}