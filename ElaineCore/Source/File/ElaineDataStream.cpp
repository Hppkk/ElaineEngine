#include "ElainePrecompiledHeader.h"
#include "ElaineDataStream.h"
#include "ElaineMemory.h"

namespace Elaine
{
	static int TranslateMode(DataStream::Mode InMode)
	{
		switch (InMode)
		{
		case Elaine::DataStream::In:
			return std::ios::in;
		case Elaine::DataStream::Out:
			return std::ios::out;
		case Elaine::DataStream::Binary:
			return std::ios::binary;
		default:
			return std::ios::in;
		}
	}

	class DataStream_Private
	{
		friend class DataStream;
	public:
		DataStream_Private(DataStream::Mode InMode)
			: mMode(InMode)
		{
		}

		~DataStream_Private()
		{
			close();
			if (mStreamData)
			{
				Memory::SystemFree(mStreamData);
				mStreamData = nullptr;
			}
		}

		void open(const std::string& path)
		{
			if (mStream.is_open())
			{
				LOG_INFO("failed to open file at {}", path);
				return;
			}

			mPath = path;
			mStream.open(path, TranslateMode(mMode));

			if (!mStream.is_open())
			{
				LOG_INFO("failed to open file at {}", path);
				return;
			}

			if (mMode & DataStream::Out)
				return;

			mStream.seekg(0, mStream.end);
			mStreamSize = mStream.tellg();
			mStreamSize += 1;
			mStream.seekg(0, mStream.beg);
		}

		void read(void* buf, size_t inSize)
		{
			if (!mStream.is_open() || mStream.eof())
				return;

			if (mPosition + inSize > mStreamSize)
				return;
			mPosition += inSize;
			mStream.read((char*)buf, inSize);
			mStream.seekg(mPosition);
		}

		void readAll()
		{
			if (!mStream.is_open())
				return;

			std::filebuf* pbuf = nullptr;
			pbuf = mStream.rdbuf();
			mStreamData = (char*)Memory::SystemMalloc(mStreamSize);
			Memory::MemoryZero(mStreamData, mStreamSize);
			pbuf->sgetn(mStreamData, mStreamSize);
		}

		void write(const void* buf, size_t inSize)
		{
			if (!mStream.is_open())
				return;

			mStream.seekp(mPosition);
			mStream.write((char*)buf, inSize);
			mPosition += inSize;
		}

		void close()
		{
			if (!mStream.is_open())
				return;

			mStream.close();
		}

		size_t getSize() const { return mStreamSize; }


	private:
		char* mStreamData = nullptr;
		size_t mStreamSize = 0u;
		size_t mPosition = 0u;
		std::string mPath = "";
		std::fstream mStream;
		DataStream::Mode mMode;
	};


	DataStream::DataStream(const std::string& InPath, Mode InMode/* = In*/)
	{
		mPrivate = new DataStream_Private(InMode);
		mPath = InPath;
		Open();
	}

	DataStream::~DataStream()
	{
		SAFE_DELETE(mPrivate);
	}

	void DataStream::Open()
	{
		mPrivate->open(mPath);
	}

	void DataStream::Close()
	{
		mPrivate->close();
	}

	void DataStream::Read(void* InBuffer, size_t InSize)
	{
		mPrivate->read(InBuffer, InSize);
	}

	void DataStream::ReadAll()
	{
		mPrivate->readAll();
	}

	void DataStream::Write(const void* InBuffer, size_t InSize)
	{
		mPrivate->write(InBuffer, InSize);
	}

	size_t DataStream::Tell() const
	{
		return mPrivate->mStreamSize;
	}

	char* DataStream::GetDataStream()
	{
		return mPrivate->mStreamData;
	}
	size_t DataStream::GetSize() const
	{
		return mPrivate->getSize();
	}
}