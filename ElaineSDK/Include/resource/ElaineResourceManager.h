#pragma once
#include "ElaineSingleton.h"
#include "ElaineResourceBase.h"

namespace Elaine
{
	class ResourceBase;
	class ElaineCoreExport ResourceManager
	{
	public:
		ResourceManager();
		virtual ~ResourceManager();
		ResourceBasePtr				createResource(const std::string& path);
		virtual	ResourceBasePtr		createResourceImpl(const std::string& path) = 0;
		ResourceBasePtr				loadOrGetResource(const std::string& path, bool async = true);

	protected:
		inline static std::string					m_sResType = "";
	private:
		std::map<std::string, ResourceBasePtr>		m_ResMap;
		
	};
}