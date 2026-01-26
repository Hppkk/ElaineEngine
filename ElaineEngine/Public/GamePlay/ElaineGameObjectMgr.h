#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineGameObject.h"

namespace Elaine
{
	class World;

	class ElaineEngineExport GameObjectMgr
	{
	public:
		GameObjectMgr(World* InWorld);
		~GameObjectMgr();
		GameObject* CreateGameObject();
		GameObject* CreateGameObjectByInfo(GameObjectInfoPtr InInfo);
		GameObject* CreateGameObjectByInfo(const std::string& path, bool async = true);
		void DestroyGameObject(GameObject* InObject);
		void DestroyAllGameObject();

	private:
		World* mWorld;
		std::set<GameObject*> m_GameObjectSet;
	};
}