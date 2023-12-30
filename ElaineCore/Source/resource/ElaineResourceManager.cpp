#include "ElainePrecompiledHeader.h"
#include "resource/ElaineResourceManager.h"
#include "resource/ElaineResourceBase.h"

namespace Elaine
{
	//std::string ResourceManager::m_sResType = GetTypeStringName(ResourceManager);

	ResourceManager::ResourceManager()
	{
	}

	ResourceManager::~ResourceManager()
	{
		//for (auto&& iter : m_ResMap)
		//{
		//	delete iter.second;
		//}
		m_ResMap.clear();
	}

	ResourceBasePtr ResourceManager::createResource(const std::string& path)
	{
		ResourceBasePtr rb = createResourceImpl(path);
		m_ResMap.emplace(path, rb);
		return rb;
	}

	ResourceBasePtr ResourceManager::loadOrGetResource(const std::string& path, bool async /*= true*/)
	{
		ResourceBasePtr res = nullptr;

		auto it = m_ResMap.find(path);
		if (it != m_ResMap.end())
		{
			res = it->second;
			if (res.isNull())
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