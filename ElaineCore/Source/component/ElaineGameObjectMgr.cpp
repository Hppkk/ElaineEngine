#include "ElainePrecompiledHeader.h"
#include "component/ElaineGameObjectMgr.h"
#include "resource/ElaineResourceManager.h"

namespace Elaine
{
	GameObjectMgr::GameObjectMgr()
	{

	}

	GameObjectMgr::~GameObjectMgr()
	{
		deleteAllGameObject();
	}

	EGameObject* GameObjectMgr::createGameObject()
	{
		EGameObject* createGo = new EGameObject();
		EGameObjectInfo* newInfo = new EGameObjectInfo();
		createGo->init(newInfo);
		m_GameObjectSet.insert(createGo);
		return createGo;
	}

	EGameObject* GameObjectMgr::createGameObjectByInfo(EGameObjectInfo* info)
	{
		EGameObject* newGo = new EGameObject();
		newGo->init(info);
		m_GameObjectSet.insert(newGo);
		return newGo;
	}

	EGameObject* GameObjectMgr::createGameObjectByInfo(const std::string& path, bool async)
	{
		EGameObjectInfo* newInfo = static_cast<EGameObjectInfo*>(GameObjectInfoMgr::instance()->createResource(path).get());
		if (async)
		{
			
		}
		else
		{
			newInfo->load();
		}

		EGameObject* newGo = new EGameObject();
		newGo->init(newInfo);
		m_GameObjectSet.insert(newGo);
		return newGo;
	}

	void GameObjectMgr::destoryGameObject(EGameObject* go)
	{
		if (go == nullptr)
			return;

		go->destroy();
	}

	void GameObjectMgr::deleteAllGameObject()
	{
		for (auto& go : m_GameObjectSet)
		{
			go->destroy();
		}
	}
}