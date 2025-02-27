#pragma once

namespace Elaine
{
	

	class ElaineCoreExport FileManager :public Singleton<FileManager>
	{
	public:
		enum FileType
		{
			FT_Shader,
			FT_Mesh,
			FT_Config,
			FT_Level,
			FT_Animation,
			FileTypeMax,
		};
	public:
		FileManager();
		~FileManager();
		void LoadFile(const std::string& InFilePath);
		const std::string& GetShaderPath(FileType InFileType) { return mFilePaths[InFileType]; }
	private:
		std::map<FileType, std::string> mFilePaths;
	};
}