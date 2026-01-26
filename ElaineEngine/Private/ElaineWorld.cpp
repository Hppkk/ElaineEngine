#include "ElainePrecompiledHeader.h"
#include "ElaineWorld.h"
#include "ElaineLevel.h"
#include "ElaineLevelManager.h"
#include "ElaineGameObject.h"
#include "ElaineTickManager.h"
#include "ElaineGameObjectMgr.h"
#include "ElaineDataStream.h"
#include "ElaineGameObjectInfoMgr.h"
#include "ElaineRenderCommandQueue.h"

namespace Elaine
{
	World::World()
	{
        mLevelManager = new LevelManager();
        mTickManager = new TickManager();
        mGameObjectMgr = new GameObjectMgr(this);

        ENQUEUE_RENDER_COMMAND(CreateSceneManager)([this](RenderContext& Context)
            {
                mSceneManager = Root::instance()->CreateSceneManager("Main SceneManager");
            });
	}

	World::~World()
	{
        ENQUEUE_RENDER_COMMAND(DestroySceneManager)([this](RenderContext& Context)
            {
                Root::instance()->DestroySceneManager(mSceneManager);
            });

        mSceneManager = nullptr;

        SAFE_DELETE(mGameObjectMgr);
        SAFE_DELETE(mLevelManager);
        SAFE_DELETE(mTickManager);
	}

	void World::Tick(float InDeltaTime)
    {
        mTickManager->RunTickGroup(TickGroup::FixedUpdate, InDeltaTime);
        mTickManager->RunTickGroup(TickGroup::Update, InDeltaTime);
        mTickManager->RunTickGroup(TickGroup::LateUpdate, InDeltaTime);


    }

    void World::LoadLevel(const std::string& InPath)
    {
        // For now treat LoadLevel as full-load (same as additive)
        LoadLevelAdditive(InPath);
    }

	Level* World::LoadLevelAdditive(const std::string& InLevelPath)
    {
        Level* NewLevel = mLevelManager->CreateLevel(this);
        NewLevel->Load(InLevelPath);

        mLevels.push_back(NewLevel);

        for (auto* GO : NewLevel->GetGameObjects())
        {
            AddToWorld(GO);
        }
        return NewLevel;
    }

    GameObject* World::CreateGameObject()
    {
        GameObject* NewGameObject = mGameObjectMgr->CreateGameObject();
        NewGameObject->Initialize();
        AddToWorld(NewGameObject);
        return NewGameObject;
    }

    void World::UnloadAllLevels()
    {
        // Unregister and destroy all loaded levels and their gameobjects
        for (auto* L : mLevels)
        {
            if (!L) continue;
            for (auto* go : L->GetGameObjects())
            {
                if (!go) continue;
                // remove from active list
                auto it = std::find(mActiveGameObjects.begin(), mActiveGameObjects.end(), go);
                if (it != mActiveGameObjects.end())
                    mActiveGameObjects.erase(it);

                mGameObjectMgr->DestroyGameObject(go);
            }
            SAFE_DELETE(L);
        }
        mLevels.clear();
        mLoadedChunks.clear();
    }

    bool World::SaveWorld(const std::string& InPath)
    {
        JsonCpp j;
        // world origin
        j["WorldOrigin"] = JsonCpp::array({ mWorldOrigin.x, mWorldOrigin.y, mWorldOrigin.z });

        // list loaded chunks (paths)
        j["LoadedChunks"] = JsonCpp::array();
        for (auto& kv : mLoadedChunks)
        {
            j["LoadedChunks"].push_back(kv.first);
        }

        std::string FullPath = Root::instance()->GetResourcePath() + InPath;
        DataStream Out(FullPath, DataStream::Out);
        std::string s = j.dump(4);
        Out.Write(s.data(), s.size());
        return true;
    }

    bool World::LoadWorld(const std::string& InPath)
    {
        std::string FullPath = Root::instance()->GetResourcePath() + InPath;
        DataStream JsonFileStream(FullPath, DataStream::Out);
        JsonFileStream.ReadAll();
        JsonCpp JsonData(JsonFileStream.GetDataStream());

        if (JsonData.is_null())
            return false;

        if (JsonData.contains("WorldOrigin"))
        {
            const auto& arr = JsonData["WorldOrigin"];
            if (arr.is_array() && arr.size() >= 3)
            {
                mWorldOrigin.x = arr[0].get<float>();
                mWorldOrigin.y = arr[1].get<float>();
                mWorldOrigin.z = arr[2].get<float>();
            }
        }

        if (JsonData.contains("Chunks"))
        {
            for (const auto& chunkNode : JsonData["Chunks"])
            {
                if (!chunkNode.contains("Path")) continue;
                std::string chunkPath = chunkNode["Path"].get<std::string>();
                bool loadNow = false;
                if (chunkNode.contains("LoadOnStart"))
                    loadNow = chunkNode["LoadOnStart"].get<bool>();

                if (loadNow)
                    LoadChunk(chunkPath);
            }
        }

        // Also support a simple LoadedChunks array
        if (JsonData.contains("LoadedChunks"))
        {
            for (const auto& p : JsonData["LoadedChunks"])
            {
                if (!p.is_string()) continue;
                LoadChunk(p.get<std::string>());
            }
        }

        return true;
    }

    Level* World::LoadChunk(const std::string& InChunkPath)
    {
        auto it = mLoadedChunks.find(InChunkPath);
        if (it != mLoadedChunks.end())
            return it->second;

        Level* NewLevel = mLevelManager->CreateLevel(this);
        if (!NewLevel->Load(InChunkPath))
        {
            SAFE_DELETE(NewLevel);
            return nullptr;
        }

        mLevels.push_back(NewLevel);
        mLoadedChunks[InChunkPath] = NewLevel;

        for (auto* GO : NewLevel->GetGameObjects())
        {
            AddToWorld(GO);
        }

        return NewLevel;
    }

    void World::UnloadChunk(const std::string& InChunkPath)
    {
        auto it = mLoadedChunks.find(InChunkPath);
        if (it == mLoadedChunks.end())
            return;

        Level* L = it->second;
        // remove gameobjects from active list and destroy them
        for (auto* go : L->GetGameObjects())
        {
            if (!go) continue;
            auto iter = std::find(mActiveGameObjects.begin(), mActiveGameObjects.end(), go);
            if (iter != mActiveGameObjects.end())
                mActiveGameObjects.erase(iter);
            mGameObjectMgr->DestroyGameObject(go);
        }

        auto lvlIt = std::find(mLevels.begin(), mLevels.end(), L);
        if (lvlIt != mLevels.end())
            mLevels.erase(lvlIt);

        mLoadedChunks.erase(it);
        SAFE_DELETE(L);
    }
    void World::RegisterTickTask(TickTask* InTask)
    {
        mTickManager->RegisterTickTask(InTask);
    }

    void World::UnregisterTickTask(TickTask* InTask)
    {
        mTickManager->UnregisterTickTask(InTask);
    }

    void World::AddToWorld(GameObject* InObject)
    {
        InObject->OnRegisterWorld(this);
        //InObject->RegisterComponents(); // ���� RenderProxy, PhysicsBody
        mActiveGameObjects.push_back(InObject);
    }
}