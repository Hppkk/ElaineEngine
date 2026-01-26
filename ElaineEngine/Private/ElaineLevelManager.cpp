#include "ElainePrecompiledHeader.h"
#include "ElaineLevelManager.h"
#include "ElaineLevel.h"

namespace Elaine
{
	LevelManager::~LevelManager()
	{

	}

	Level* LevelManager::CreateLevel(World* InWorld)
	{
		return new Level(InWorld);
	}

	void LevelManager::DestroyLevel(Level* InLevel)
	{
		if (InLevel == nullptr)
			return;

		mLevels.insert(InLevel);
	}
}