#include "ElainePrecompiledHeader.h"
#include "resource/ElaineResourceBase.h"

namespace Elaine
{
	ResourceBase::ResourceBase(const std::string& res_name) :m_sResName(res_name)
	{

	}

	ResourceBase::~ResourceBase()
	{
		unload();
	}

	void ResourceBase::load()
	{
		try
		{
			loadImpl();
			++m_nMemoryUsed;
		}
		catch (...)
		{
			//throw();
		}
	}

	void ResourceBase::unload()
	{
		if (m_nMemoryUsed == 0)
			return;

		--m_nMemoryUsed;
	}

	void ResourceBase::asyncLoad()
	{
		auto resThread = ThreadManager::instance()->getThread("ResourceThread");
		if (resThread != nullptr)
		{
			ThreadEventDesc evenDesc{};
			evenDesc.init(&ResourceBase::load, this);
			resThread->pushThreadFunc(evenDesc);
			//todo  如果线程阻塞，通知线程开启
		}
	}
}