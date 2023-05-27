#pragma once
#include "ElaineStdRequirements.h"

namespace Elaine
{
	class EComponent;
	class EGameObject
	{
	public:
		EGameObject();
		~EGameObject();
		EComponent*								GetComponentByName(const std::string& name);
		std::vector<EComponent*>&				GetComponents() { return m_components; }
	private:
		std::vector<EComponent*>						m_components;
		std::map<std::string, EComponent*>				m_componentsMap;
	};
}
