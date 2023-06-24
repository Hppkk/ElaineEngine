#include "ElainePrecompiledHeader.h"
#include "resource/ElaineResourceBase.h"

namespace Elaine
{
	ResourceListener::ResourceListener()
	{

	}

	void ResourceListener::requestArrived()
	{

	}


	void ResourceListener::requestResource()
	{

	}



	ResourceBase::ResourceBase(const std::string& res_name) :m_sResName(res_name)
	{

	}

	ResourceBase::~ResourceBase()
	{
		unload();
	}

	void ResourceBase::load(bool async/* = true*/)
	{
		loadImpl();
		m_uUseMemory++;
	}

	void ResourceBase::loadOrObtainResource(bool async/* = true*/)
	{
		if (getUseCount() != 0)
		{
			m_uUseMemory++;
			return;
		}

		try
		{
			if (async)
			{
				asyncLoad();
			}
			else
			{
				loadImpl();
			}
			m_uUseMemory++;
		}
		catch (...)
		{
			throw std::exception(("file: " + m_sResName + "not found!").c_str());
		}
	}

	void ResourceBase::unload()
	{
		unloadImpl();
	}

	void ResourceBase::reload()
	{
		unload();
		load();
	}

	void ResourceBase::asyncLoad()
	{
		auto resThread = ThreadManager::instance()->getThread("ResourceThread");
		if (resThread != nullptr)
		{
			ThreadEventDesc evenDesc{};
			evenDesc.init(&ResourceBase::loadImpl, this);
			resThread->pushThreadFunc(evenDesc);
			//todo  如果线程阻塞，通知线程开启
		}
	}
}