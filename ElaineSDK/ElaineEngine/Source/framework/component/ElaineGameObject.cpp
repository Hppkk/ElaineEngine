#include "framework/component/ElaineGameObject.h"
#include "framework/component/ElaineComponent.h"

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
}