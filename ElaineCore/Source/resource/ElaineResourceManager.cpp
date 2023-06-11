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

	void ResourceManager::load(const std::string& path)
	{
		auto it = m_ResMap.find(path);
		if (it == m_ResMap.end())
		{
			
		}
	}

	void ResourceManager::update()
	{
		for (auto it = m_ResMap.begin(); it != m_ResMap.end(); ++it)
		{
			if (it->second->m_nMemoryUsed == 0)
			{
				delete it->second;
				it = m_ResMap.erase(it);
			}
		}
	}
}