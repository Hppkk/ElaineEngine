#include "ElainePrecompiledHeader.h"
#include "resource/ElaineResourceBase.h"

namespace Elaine
{
	ResourceBase::ResourceBase(const std::string& res_name)
	{

	}

	ResourceBase::~ResourceBase()
	{
		unload();
	}

	void ResourceBase::load()
	{
		loadImpl();
		++m_nMemoryUsed;
	}

	void ResourceBase::unload()
	{
		if (m_nMemoryUsed == 0)
			return;

		--m_nMemoryUsed;
	}

	void ResourceBase::loadImpl()
	{

	}

	void ResourceBase::unloadImpl()
	{

	}
}