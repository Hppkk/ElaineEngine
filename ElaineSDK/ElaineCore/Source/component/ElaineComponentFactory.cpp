#include "ElainePrecompiledHeader.h"
#include "component/ElaineComponentFactory.h"
#include "component/ElaineComponent.h"

namespace Elaine
{
	void ComponentFactory::destoryComponent(EComponent* pCom)
	{
		if (pCom == nullptr)
			return;

		auto iter = m_ComSet.find(pCom);
		if (iter == m_ComSet.end())
			return;

		m_ComSet.erase(iter);
		
		SAFE_DELETE(pCom)
	}

	void ComponentFactory::destoryComponentInfo(EComponentInfo* pInfo)
	{
		if (pInfo == nullptr)
			return;

		auto iter = m_ComInfoSet.find(pInfo);
		if (iter == m_ComInfoSet.end())
			return;

		m_ComInfoSet.erase(iter);

		SAFE_DELETE(pInfo)
	}

	void ComponentFactory::destoryAllComponent()
	{

	}

	ComponentFactory::~ComponentFactory()
	{
		for (auto com : m_ComSet)
		{
			SAFE_DELETE(com);
		}
		for (auto info : m_ComInfoSet)
		{
			SAFE_DELETE(info)
		}
		m_ComSet.clear();
		m_ComInfoSet.clear();
	}

	ComponentFactoryManager::~ComponentFactoryManager()
	{
		for (auto&& factory : m_ComFactoryMap)
		{
			SAFE_DELETE(factory.second);
		}
		m_ComFactoryMap.clear();
	}

	ComponentFactory* ComponentFactoryManager::getFactoryByComType(const std::string& comType)
	{
		auto it = m_ComFactoryMap.find(comType);
		if (it == m_ComFactoryMap.end())
			return nullptr;

		return it->second;
	}

	void ComponentFactoryManager::registerComFactory(const std::string& comType, ComponentFactory* factory)
	{
		auto it = m_ComFactoryMap.find(comType);
		if (it == m_ComFactoryMap.end())
		{
			m_ComFactoryMap[comType] = factory;
		}
	}

}