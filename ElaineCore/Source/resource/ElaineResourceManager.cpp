#include "ElainePrecompiledHeader.h"
#include "resource/ElaineResourceManager.h"
#include "resource/ElaineResourceBase.h"

namespace Elaine
{
	ResourceManager::ResourceManager()
	{
		m_sResType = GetTypeStringName(ResourceManager);
	}

	ResourceManager::~ResourceManager()
	{
		for (auto&& iter : m_ResMap)
		{
			delete iter.second;
		}
		m_ResMap.clear();
	}

	ResourceBase* ResourceManager::loadOrGetResource(const std::string& path, bool async /*= true*/)
	{
		ResourceBase* res = nullptr;

		auto it = m_ResMap.find(path);
		if (it != m_ResMap.end())
		{
			res = it->second;
			if (res == nullptr)
			{
				res = createResource(path);
				res->load(async);
				m_ResMap[path] = res;
			}
		}
		else
		{
			res = createResource(path);
			res->load(async);
			m_ResMap[path] = res;
		}
		return res;
	}

}