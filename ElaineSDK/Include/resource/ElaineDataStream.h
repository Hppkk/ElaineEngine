#pragma once
#include "ElaineCorePrerequirements.h"
#include "resource/ElaineResourceBase.h"
#include "resource/ElaineResourceManager.h"

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
	using DataStreamPtr = ResourcePtr<DataStream>;

	class ElaineCoreExport DataStream :public ResourceBase
	{
	public:
		DataStream(ResourceManager* pManager, const std::string& res_name, DataStreamSource dss);
		virtual ~DataStream() override;
		virtual void			loadImpl() override;
		virtual	void			unloadImpl() override;
		virtual void			loadOrObtainResource(bool async = true) override;
		char*					getDataStream();
	protected:
		DataStream_Private*	mpData = nullptr;
		DataStreamSource mDataStreamSource = DSS_None;
	};
}