#include "ElainePrecompiledHeader.h"
#include "component/ElaineComponent.h"

namespace Elaine
{
	std::string EComponent::m_sType = "EComponent";

	EComponentInfo::EComponentInfo()
	{

	}

	EComponentInfo::~EComponentInfo()
	{

	}

	void EComponentInfo::exportData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;

		exportDataImpl(jsonNode);
	}

	void EComponentInfo::importData(cJSON* jsonNode)
	{
		if (jsonNode == nullptr)
			return;

		importDataImpl(jsonNode);
	}

	void EComponentInfo::exportDataImpl(cJSON* jsonNode)
	{
	}

	void EComponentInfo::importDataImpl(cJSON* jsonNode)
	{
	}

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