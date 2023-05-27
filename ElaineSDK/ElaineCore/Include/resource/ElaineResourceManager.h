#pragma once
#include "ElaineSingleton.h"

namespace Elaine
{
	class ResourceBase;
	class ElaineCoreExport ResourceManager :public Singleton<ResourceManager>
	{
	public:
		ResourceManager();
		~ResourceManager();
		void				load(const std::string& path);
		void				update();
	private:
		std::map<std::string, ResourceBase*>		m_ResMap;
	};
}