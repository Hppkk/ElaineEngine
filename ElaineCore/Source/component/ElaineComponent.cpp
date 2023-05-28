#include "ElainePrecompiledHeader.h"
#include "component/ElaineComponent.h"

namespace Elaine
{
	std::string EComponent::m_sType = "EComponent";

	EComponent::EComponent()
		:m_pParentGameObject(nullptr)
	{

	}
	EComponent::~EComponent()
	{

	}

	void EComponent::init(EComponentInfo* info)
	{

	}
}