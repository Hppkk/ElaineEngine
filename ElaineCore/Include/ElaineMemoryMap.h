#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{

	class ElaineCoreExport MemoryMapFile
	{
	public:
		enum FileMode
		{
			ReadOnly,
			WriteOnly,
			ReadAndWrite,
		};
		MemoryMapFile(const std::string& InPath, FileMode InMode = ReadOnly);
		~MemoryMapFile();
		void UnMap();
		void* MapPointer() const { return mMapHandle; }
		bool Read(void* InBuffer, size_t InReadSize);
		bool Write(void* InBuffer, size_t InWriteSize);
		size_t MapSize() { return mMapSize; }
	private:
		void* mMapHandle = nullptr;
		size_t mMapSize = 0u;
		size_t mPointer = 0u;
	};
}