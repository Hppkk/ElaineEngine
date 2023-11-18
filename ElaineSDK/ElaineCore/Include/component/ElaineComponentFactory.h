#pragma once
#include "ElaineSingleton.h"

namespace Elaine
{
	class EComponent;
	class EComponentInfo;

	class ElaineCoreExport ComponentFactory
	{
	public:
		ComponentFactory() = default;
		
		virtual ~ComponentFactory();
		virtual EComponent*				createComponent() = 0;
		virtual EComponentInfo*			createComponentInfo() = 0;
		void							destoryComponent(EComponent* pCom);
		void							destoryComponentInfo(EComponentInfo* pInfo);
		void							destoryAllComponent();
	protected:
		std::string						m_sComType;
		std::set<EComponent*>			m_ComSet;
		std::set<EComponentInfo*>		m_ComInfoSet;
	};

	class ElaineCoreExport ComponentFactoryManager :public Singleton<ComponentFactoryManager>
	{
	public:
		ComponentFactoryManager() = default;
		~ComponentFactoryManager();
		ComponentFactory*			getFactoryByComType(const std::string& comType);
		void						registerComFactory(const std::string& comType, ComponentFactory* factory);
	private:
		std::map<std::string, ComponentFactory*>		m_ComFactoryMap;
	};
}