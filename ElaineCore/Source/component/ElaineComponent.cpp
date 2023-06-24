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
		if (m_bIsTickCom)
			ComponentTickManager::instance()->unRegisterTickComponent(this);
	}

	void EComponent::init(EComponentInfo* info)
	{

	}

	void EComponent::registerTickTime(TickTime tt)
	{
		m_tickTimeSet.insert(tt);
	}

	void EComponent::registerComNeedTick()
	{
		m_bIsTickCom = true;
		ComponentTickManager::instance()->registerTickComponent(this);
	}

	ComponentTickManager::~ComponentTickManager()
	{

	}

	void ComponentTickManager::registerTickComponent(EComponent* com)
	{
		m_components.insert(com);
	}

	void ComponentTickManager::unRegisterTickComponent(EComponent* com)
	{
		if (m_components.find(com) == m_components.end())
			return;

		m_components.erase(com);
	}

	void ComponentTickManager::tick(float dt)
	{
		for (auto com : m_components)
		{
			for (auto tt : com->m_tickTimeSet)
			{
				switch (tt)
				{
				case beginFrame:
					com->beginFrameTick(dt);
					break;
				case fixedUpdate:
					com->fixedUpdate(dt);
					break;
				case animatorUpdate:
					com->animatorUpdate(dt);
					break;
				case endFrame:
					com->endFrame(dt);
					break;
				default:
					break;
				}
			}
		}
	}
}