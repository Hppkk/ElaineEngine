#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineGameObjectMgr.h"
#include "ElaineResourceManager.h"
#include "ElaineGameObjectInfoMgr.h"

namespace Elaine
{
	GameObjectMgr::GameObjectMgr(World* InWorld)
		: mWorld(InWorld)
	{

	}

	GameObjectMgr::~GameObjectMgr()
	{
		DestroyAllGameObject();
	}

	GameObject* GameObjectMgr::CreateGameObject()
	{
		GameObject* createGo = new GameObject(mWorld);
		m_GameObjectSet.insert(createGo);
		return createGo;
	}

	GameObject* GameObjectMgr::CreateGameObjectByInfo(GameObjectInfoPtr InInfo)
	{
		GameObject* newGo = new GameObject(mWorld);
		newGo->Initialize(InInfo);
		m_GameObjectSet.insert(newGo);
		return newGo;
	}

	GameObject* GameObjectMgr::CreateGameObjectByInfo(const std::string& path, bool async)
	{
		GameObjectInfoPtr NewInfo = GameObjectInfoMgr::instance()->CreateEmptyResource(path);
		if (async)
		{
			
		}
		else
		{
			NewInfo->LoadResource();
		}

		GameObject* newGo = new GameObject(mWorld);
		newGo->Initialize(NewInfo);
		m_GameObjectSet.insert(newGo);
		return newGo;
	}

	void GameObjectMgr::DestroyGameObject(GameObject* InObject)
	{
		if (InObject == nullptr)
			return;

		InObject->Destroy();
		SAFE_DELETE(InObject);
	}

	void GameObjectMgr::DestroyAllGameObject()
	{
		for (auto& go : m_GameObjectSet)
		{
			go->Destroy();
		}
	}
}