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
		virtual	ResourceBase*		createResource(const std::string& path);
		void						load(const std::string& path);
		void						update();
	protected:
		std::string									m_sResType;
	private:
		std::map<std::string, ResourceBase*>		m_ResMap;
		
	};
}