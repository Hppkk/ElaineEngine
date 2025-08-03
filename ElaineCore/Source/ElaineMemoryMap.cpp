#include "ElainePrecompiledHeader.h"
#include "ElaineMemoryMap.h"

namespace Elaine
{
	MemoryMapFile::MemoryMapFile(const std::string& InPath, FileMode InMode)
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
        HANDLE hFile = CreateFileA(
            InPath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            assert(false);
            LOG_ERROR("CreateFile failed!");
            return;
        }

        mMapSize = GetFileSize(hFile, NULL);
        if (mMapSize == INVALID_FILE_SIZE)
        {
            assert(false);
            LOG_ERROR("GetFileSize failed.");
            CloseHandle(hFile);
            return;
        }

        DWORD fileMode = PAGE_READONLY;
        DWORD mapMode = FILE_MAP_READ;
        switch (InMode)
        {
        case Elaine::MemoryMapFile::ReadOnly:
            fileMode = PAGE_READONLY;
            mapMode = FILE_MAP_READ;
            break;
        case Elaine::MemoryMapFile::WriteOnly:
            fileMode = PAGE_READWRITE;
            mapMode = FILE_MAP_WRITE;
            break;
        case Elaine::MemoryMapFile::ReadAndWrite:
            fileMode = PAGE_READWRITE;
            mapMode = FILE_MAP_ALL_ACCESS;
            break;
        default:
            break;
        }

        HANDLE hMapFile = CreateFileMappingA(
            hFile,
            NULL,
            fileMode,
            0,
            0,
            NULL);

        if (hMapFile == NULL)
        {
            LOG_ERROR("CreateFileMapping failed.");
            CloseHandle(hFile);
            return;
        }

        mMapHandle = MapViewOfFile(
            hMapFile,
            mapMode,
            0,
            0,
            mMapSize);

        if (mMapHandle == NULL)
        {
            LOG_ERROR("MapViewOfFile failed.");
            CloseHandle(hMapFile);
            CloseHandle(hFile);
            return;
        }

        CloseHandle(hMapFile);
        CloseHandle(hFile);
#endif
	}

	MemoryMapFile::~MemoryMapFile()
	{
        UnMap();
	}

	void MemoryMapFile::UnMap()
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
        UnmapViewOfFile(mMapHandle);
#endif
	}

	bool MemoryMapFile::Read(void* InBuffer, size_t InReadSize)
	{
        char* cMapHandle = static_cast<char*>(mMapHandle);
        if (mPointer >= mMapSize)
        {
            return false;
        }

        size_t realReadSize = (mMapSize - mPointer < InReadSize) ? mMapSize - mPointer : InReadSize;

        Memory::MemoryCopy(InBuffer, cMapHandle + mPointer, realReadSize);
		return true;
	}

	bool MemoryMapFile::Write(void* InBuffer, size_t InWriteSize)
	{
		return false;
	}
}