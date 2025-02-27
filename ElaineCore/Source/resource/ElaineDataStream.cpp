#include "ElainePrecompiledHeader.h"
#include "resource/ElaineDataStream.h"
#include "ElaineMemory.h"

namespace Elaine
{
	class DataStream_Private
	{
		friend class DataStream;
	public:

		

		~DataStream_Private()
		{
			if (m_pData)
			{
				Memory::SystemFree(m_pData);
				m_pData = nullptr;
			}
		}

		void open(const std::string& path)
		{
			m_sPath = path;
			m_stream.open(path, std::ios::binary);
			if (!m_stream.is_open())
			{
				LOG_INFO("failed to open file at " + path);
				return;
			}
			m_stream.seekg(0, m_stream.end);
			m_uSize = m_stream.tellg();
			m_uSize += 1;
			m_stream.seekg(0, m_stream.beg);
		}

		void read()
		{
			if (!m_stream.is_open())
				return;

			std::filebuf* pbuf = nullptr;
			pbuf = m_stream.rdbuf();
			m_pData = (char*)Memory::SystemMalloc(m_uSize);
			Memory::MemoryZero(m_pData, m_uSize);
			pbuf->sgetn(m_pData, m_uSize);

		}

		void close()
		{
			if (!m_stream.is_open())
				return;

			m_stream.close();
		}

		size_t getSize() { return m_uSize; }


	private:
		char* m_pData = nullptr;
		size_t   m_uSize = 0u;
		std::string m_sPath = "";
		std::ifstream m_stream;
	};


	DataStream::DataStream(ResourceManager* pManager, const std::string& res_name, DataStreamSource dss)
		: ResourceBase(pManager, res_name)
		, mDataStreamSource(dss)
	{
		mpData = new DataStream_Private();
	}

	DataStream::~DataStream()
	{
		SAFE_DELETE(mpData);
	}

	void DataStream::loadImpl()
	{
		if (msResName.empty())
			return;

		mpData->open(msResName);
		mpData->read();
		mpData->close();
	}

	void DataStream::unloadImpl()
	{

	}

	void DataStream::loadOrObtainResource(bool async)
	{

	}

	char* DataStream::getDataStream()
	{
		return mpData->m_pData;
	}
}