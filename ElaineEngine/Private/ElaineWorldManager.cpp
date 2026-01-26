#include "ElainePrecompiledHeader.h"
#include "ElaineWorldManager.h"
#include "ElaineWorld.h"

namespace Elaine
{
	WorldManager::WorldManager()
	{

	}

	WorldManager::~WorldManager()
	{

	}

	World* WorldManager::CreateWorld()
	{
		World* NewWorld = new World();
		mWorlds.insert(NewWorld);
		return NewWorld;
	}

	void WorldManager::DestroyWorld(World* InWorld)
	{
		if (InWorld == nullptr)
			return;

		mWorlds.erase(InWorld);
		SAFE_DELETE(InWorld);
	}
}