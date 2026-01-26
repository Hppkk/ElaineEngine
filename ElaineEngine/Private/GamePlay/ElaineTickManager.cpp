#include "ElainePrecompiledHeader.h"
#include "ElaineTickManager.h"

namespace Elaine
{
	TickManager::TickManager()
	{

	}

	TickManager::~TickManager()
	{

	}

	void TickManager::RegisterTickTask(TickTask* InTask)
	{
		if (!InTask->bCanEverTick) return;

		int GroupIdx = (int)InTask->mTickGroup;
		mTickLists[GroupIdx].push_back(InTask);
	}

	void TickManager::UnregisterTickTask(TickTask* InTask)
	{
		int GroupIdx = (int)InTask->mTickGroup;
		auto& GroupList = mTickLists[GroupIdx];

		for (size_t i = 0; i < GroupList.size(); ++i)
		{
			if (GroupList[i] == InTask)
			{
				if (i != GroupList.size() - 1)
				{
					std::swap(GroupList[i], GroupList.back());
				}
				GroupList.pop_back();
				return;
			}
		}
	}

	void TickManager::RunTickGroup(TickGroup InGroup, float InDeltaTime)
	{
		auto& GroupList = mTickLists[(int)InGroup];

		//TODO: Prevent iterator invalidation.
		for (size_t i = 0; i < GroupList.size(); ++i)
		{
			TickTask* CurrentTask = GroupList[i];
			if (CurrentTask->bIsTickEnabled)
			{
				CurrentTask->Execute(InDeltaTime);
			}
		}
	}
}