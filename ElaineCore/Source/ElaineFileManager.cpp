#include "ElainePrecompiledHeader.h"
#include "ElaineFileManager.h"

namespace Elaine
{
	FileManager::FileManager()
	{
		mFilePaths[FT_Shader] = "shader/vulkan";
	}

	FileManager::~FileManager()
	{

	}

	void FileManager::LoadFile(const std::string& InFilePath)
	{

	}
}
