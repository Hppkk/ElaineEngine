#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	enum DataStreamSource
	{
		DSS_None,
		DSS_Memory,
		DSS_File,
	};

	class DataStream_Private;
	class DataStream;


	class ElaineCoreExport DataStream
	{
	public:
		enum Mode
		{
			In = 1,
			Out = 1 << 1,
			Binary = 1 << 2,
		};
	public:
		DataStream(const std::string& InPath, Mode InMode = In);
		~DataStream();
		void Open();
		void Close();
		void ReadAll();
		template<typename T>
		void Read(T& OutData)
		{
			static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable.");
			Read(&OutData, sizeof(T));
		}

		void Read(void* InBuffer, size_t InSize);
		template<typename T>
		void Write(const T& InData)
		{
			static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable.");
			Write(&InData, sizeof(T));
		}
		void Write(const void* InBuffer, size_t InSize);

		size_t Tell() const;
		char* GetDataStream();
		size_t GetSize() const;
	protected:
		DataStream_Private*	mPrivate = nullptr;
		DataStreamSource mDataSource = DSS_None;
		std::string mPath;
	};

	ENUM_OPERATORS(DataStream::Mode);
}