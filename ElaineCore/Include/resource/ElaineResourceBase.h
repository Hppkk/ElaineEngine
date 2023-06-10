#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	enum RES_LOADPRIORITY : char
	{

	};

	enum ResType
	{
		rt_Mesh,
		rt_Partical,
		rt_Texture,
	};

	class ElaineCoreExport ResourceBase
	{
		friend class ResourceManager;
	public:
		ResourceBase() = default;
		ResourceBase(const std::string& res_name);
		virtual ~ResourceBase();
		void					load();
		void					unload();
		virtual void			loadImpl() = 0;
		virtual	void			unloadImpl() = 0;
		void					asyncLoad();
	protected:
		uint32_t				m_nMemoryUsed = 0;	//引用计数
		std::string				m_sResName;	//资源路径
	};
}