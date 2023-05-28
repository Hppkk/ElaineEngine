#include "ElainePrecompiledHeader.h"
#include "component/ElaineGameObject.h"
#include "component/ElaineComponent.h"

namespace Elaine
{
	EGameObjectInfo::EGameObjectInfo()
	{

	}

	EGameObjectInfo::EGameObjectInfo(const std::string& path) :ResourceBase(path)
	{

	}

	EGameObjectInfo::~EGameObjectInfo()
	{

	}

	void EGameObjectInfo::loadImpl()
	{

	}

	void EGameObjectInfo::unloadImpl()
	{

	}



	EGameObject::EGameObject()
	{

	}

	EGameObject::EGameObject(const std::string& name)
	{

	}

	EGameObject::~EGameObject()
	{

	}

	void EGameObject::init(EGameObjectInfo* info)
	{

	}

	EComponent* EGameObject::GetComponentByName(const std::string& name)
	{
		auto it = m_componentsMap.find(name);
		if (it != m_componentsMap.end())
			return (*it).second;
		return nullptr;
	}

	void EGameObject::AddChildGameObject(EGameObject* obj)
	{

	}

	void EGameObject::AddComponents(EComponent* com)
	{

	}

	EGameObject* EGameObject::CreateChildGameObject()
	{
		return nullptr;
	}

	void EGameObject::destroy()
	{

	}

	void EGameObject::removeComponent(EComponent* rhs)
	{
		if (rhs == nullptr)
			return;

		auto it = m_componentsIndexMap.find(rhs);
		if (it == m_componentsIndexMap.end())
			return;
		EComponent* removeCom = it->first;
		size_t		idx = it->second;
		m_componentsIndexMap.erase(it);
		auto iter = m_components.begin() + idx;
		m_components.erase(iter);
		
		auto factory = ComponentFactoryManager::instance()->getFactoryByComType(rhs->m_sType);
		if (factory == nullptr)
			return;

		factory->destoryComponent(removeCom);
	}

	void EGameObject::removeChildGameObject(EGameObject* rhs)
	{
		if (rhs == nullptr)
			return;
		auto it = m_childGameObjectIdxMap.find(rhs);
		if (it == m_childGameObjectIdxMap.end())
			return;

		size_t idx = it->second;
		auto iter = m_childGameObjects.begin() + idx;
		m_childGameObjects.erase(iter);

	}
}