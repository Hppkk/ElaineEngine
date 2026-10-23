#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineActorManager.h"
#include "ElaineResourceManager.h"
#include "ElaineActorInfoMgr.h"
#include "ElaineWorld.h"

namespace Elaine
{
	ActorManager::ActorManager(World* InWorld)
		: mWorld(InWorld)
	{

	}

	ActorManager::~ActorManager()
	{
		DestroyAllActor();
	}

	Actor* ActorManager::CreateActor()
	{
		MemoryScope Scope({Memory::TypeId("Actor"), 0, 0});
		Actor* createGo = new Actor(mWorld);
		m_ActorSet.insert(createGo);
		return createGo;
	}

	Actor* ActorManager::CreateActorByInfo(ActorInfoPtr InInfo)
	{
		MemoryScope Scope({Memory::TypeId("Actor"), 0, 0});
		Actor* newGo = new Actor(mWorld);
		newGo->Initialize(InInfo);
		m_ActorSet.insert(newGo);
		return newGo;
	}

    Actor* ActorManager::CreateActorByInfo(const std::string& Path)
    {
        auto Info = ActorInfoMgr::instance()->GetResource(Path);
        if (!Info || !Info->IsReady()) return nullptr;
        return CreateActorByInfo(Info);
    }

	void ActorManager::DestroyActor(Actor* InObject)
	{
		if (InObject == nullptr)
			return;

		m_ActorSet.erase(InObject);
		if (mWorld)
			mWorld->RemoveFromWorld(InObject);
		else
			InObject->OnUnregisterWorld();
		InObject->Destroy();
		SAFE_DELETE(InObject);
	}

	void ActorManager::DestroyAllActor()
	{
		while (!m_ActorSet.empty())
		{
			Actor* CurrentActor = *m_ActorSet.begin();
			DestroyActor(CurrentActor);
		}
	}
}
