#pragma once
#include "ElaineSingleton.h"
#include "ElaineGameObject.h"

namespace Elaine
{
	class ElaineCoreExport GameObjectMgr :public Singleton<GameObjectMgr>
	{
	public:
		GameObjectMgr();
		~GameObjectMgr();
		EGameObject*		createGameObject();
		EGameObject*		createGameObjectByInfo(EGameObjectInfo* info);
		EGameObject*		createGameObjectByInfo(const std::string& path, bool async = true);
		void				destoryGameObject(EGameObject* go);
		void				deleteAllGameObject();

	private:
		std::set<EGameObject*>		m_GameObjectSet;
		
	};
}