#include "ElainePrecompiledHeader.h"
#include "component/ElaineGameObject.h"
#include "component/ElaineComponent.h"

namespace Elaine
{
	EGameObject::EGameObject()
	{

	}

	EGameObject::~EGameObject()
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

	}
}