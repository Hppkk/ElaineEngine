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



	ResourceBase::ResourceBase(ResourceManager* pManager, const std::string& res_name)
		: msResName(res_name)
		, mManager(pManager)
	{

	}

	ResourceBase::~ResourceBase()
	{
		unload();
	}

	void ResourceBase::load(bool async/* = true*/)
	{
		loadImpl();

	}

	void ResourceBase::loadOrObtainResource(bool async/* = true*/)
	{


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

		}
		catch (...)
		{
			throw std::exception(("file: " + msResName + "not found!").c_str());
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

	}
}