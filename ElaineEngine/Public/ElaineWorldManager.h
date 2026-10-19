#pragma once
#include "ElaineEnginePrerequirements.h"

namespace Elaine
{
	class World;

	class ElaineEngineExport WorldManager
	{
	public:
		WorldManager();
		~WorldManager();
		World* CreateWorld();
		void DestroyWorld(World* InWorld);
		void RenderOneFrame(float InDeltaTime = 0.0f);
	private:
		std::set<World*> mWorlds;
	};
}