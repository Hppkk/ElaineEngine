#include "ElainePrecompiledHeader.h"
#include "resource/ElaineDataStreamMgr.h"

namespace Elaine
{
	//std::string DataStreamMgr::m_sResType = GetTypeStringName(DataStream);

	DataStreamMgr::~DataStreamMgr()
	{

	}

	ResourceBasePtr DataStreamMgr::createResourceImpl(const std::string& path)
	{
		return new DataStream(this, path, DSS_File);
	}

	ResourceBasePtr DataStreamMgr::getDataStreamFromFile(const std::string& path, bool async)
	{
		ResourceBasePtr res = createResource(path);
		res->load(async);
		return res;
	}

	ResourceBasePtr DataStreamMgr::getDataStreamFromMemory(const std::string& name)
	{
		return nullptr;
	}
}