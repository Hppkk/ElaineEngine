#pragma once
#include "ElaineCorePrerequirements.h"
#include "resource/ElaineResourceManager.h"
#include "resource/ElaineDataStream.h"

namespace Elaine
{
	
	class ElaineCoreExport DataStreamMgr :public ResourceManager, public Singleton<DataStreamMgr>
	{
	public:
		virtual ~DataStreamMgr() override;
		virtual	ResourceBasePtr createResourceImpl(const std::string& path) override;
		ResourceBasePtr getDataStreamFromFile(const std::string& path, bool async = true);
		ResourceBasePtr getDataStreamFromMemory(const std::string& name);
	};
}