#pragma once
#include "ElaineEnginePrerequirements.h"

namespace Elaine
{
	enum class TickGroup : uint8_t
	{
		FixedUpdate,
		Update,
		LateUpdate,
		TickCount,
	};

	struct TickTask
	{
		virtual ~TickTask() = default;
		virtual void Execute(float InDeltaTime) = 0;
		TickGroup mTickGroup = TickGroup::Update;
		bool bCanEverTick = true;
		bool bIsTickEnabled = true;
	};

	template<typename ComponentType>
	struct TComponentTickTask : public TickTask
	{
		using TickSignature = void(ComponentType::*)(float);

		virtual void Execute(float InDeltaTime) override
		{
			if (mTarget && bIsTickEnabled)
			{
				(mTarget->*mMethod)(InDeltaTime);
			}
		}

		void Bind(ComponentType* InTarget, TickSignature InMethod, TickGroup InGroup)
		{
			mTarget = InTarget;
			mMethod = InMethod;
			mTickGroup = InGroup;
		}

		ComponentType* mTarget = nullptr;
		TickSignature mMethod = nullptr;
	};

	class ElaineEngineExport TickManager
	{
	public:
		TickManager();
		~TickManager();
		void RegisterTickTask(TickTask* InTask);
		void UnregisterTickTask(TickTask* InTask);
		void RunTickGroup(TickGroup InGroup, float InDeltaTime);
	private:
		std::vector<TickTask*> mTickLists[(int)TickGroup::TickCount];
	};
}