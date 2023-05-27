#include "ElainePrecompiledHeader.h"
#include "component/ElaineComponentFactory.h"
#include "component/ElaineComponent.h"

namespace Elaine
{
	ComponentFactory::~ComponentFactory()
	{
		for (auto com : m_ComSet)
		{
			SAFE_DELETE(com);
		}
		m_ComSet.clear();
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