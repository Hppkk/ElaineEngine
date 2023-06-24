#pragma once
#include "ElaineSingleton.h"

namespace Elaine
{
	class ResourceBase;
	class ElaineCoreExport ResourceManager :public Singleton<ResourceManager>
	{
	public:
		ResourceManager();
		virtual ~ResourceManager();
		virtual	ResourceBase*		createResource(const std::string& path) = 0;
		ResourceBase*				loadOrGetResource(const std::string& path, bool async = true);
	protected:
		inline static std::string					m_sResType = "";
	private:
		std::map<std::string, ResourceBase*>		m_ResMap;
		
	};
}