#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineActor.h"

namespace Elaine
{
	class World;

	class ElaineEngineExport ActorManager
	{
	public:
		ActorManager(World* InWorld);
		~ActorManager();
		Actor* CreateActor();
		Actor* CreateActorByInfo(ActorInfoPtr InInfo);
		Actor* CreateActorByInfo(const std::string& path, bool async = true);
		void DestroyActor(Actor* InObject);
		void DestroyAllActor();

	private:
		World* mWorld;
		std::set<Actor*> m_ActorSet;
	};
}