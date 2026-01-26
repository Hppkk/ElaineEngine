#pragma once
#include "ElaineEnginePrerequirements.h"

namespace Elaine
{
	class Level;
	class World;

	class ElaineEngineExport LevelManager
	{
	public:
		LevelManager() = default;
		~LevelManager();
		Level* CreateLevel(World* InWorld);
		void DestroyLevel(Level* InLevel);
	private:
		std::set<Level*> mLevels;
	};
}