#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourcePtr.h"

namespace Elaine
{
	enum ResLoadPriority
	{
		ResLoadPriority_Low,
		ResLoadPriority_Mid,
		ResLoadPriority_High,
	};

	enum ResType
	{
		rt_Mesh = ResLoadPriority_Mid,
		rt_Partical = ResLoadPriority_Low,
		rt_Texture = ResLoadPriority_Low,
		rt_DataBase = ResLoadPriority_High,
		rt_Other = ResLoadPriority_Mid,

	};

	class ResourceBase;
	using ResourceBasePtr = ResourcePtr<ResourceBase>;
	class ElaineCoreExport ResourceListener
	{
	public:
		ResourceListener();
		~ResourceListener() = default;
		void requestArrived();
		void		requestResource();
	private:
		ResourceBasePtr		m_res;
	};

	using ResourceListenerSet = std::set<ResourceListener*>;

	class ElaineCoreExport ResourceBase 
	{
		friend class ResourceManager;

	public:
		ResourceBase() = default;
		ResourceBase(ResourceManager* pManager, const std::string& res_name);
		virtual ~ResourceBase();
		void					load(bool async = true);
		void					unload();
		void					reload();
		virtual void			loadImpl() = 0;
		virtual	void			unloadImpl() = 0;
		void					asyncLoad();
		virtual void			loadOrObtainResource(bool async = true);
	protected:
		std::string				msResName;	//×ÊÔ´Â·¾¶
		ResourceManager*		mManager = nullptr;
		unsigned long			mUMemorySize = 0;
	};
}