#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineVector3.h"

namespace Elaine
{
    class Level;
    class LevelManager;
    class GameObject;
    class TickManager;
    struct TickTask;
    class GameObjectMgr;
    class SceneManager;

    class ElaineEngineExport World
    {
    public:
        World();
        ~World();
        void Tick(float InDeltaTime);
        // World-level persist/load
        bool SaveWorld(const std::string& InPath);
        bool LoadWorld(const std::string& InPath);
        // Chunked loading: load a chunk (level file) on demand
        Level* LoadChunk(const std::string& InChunkPath);
        void UnloadChunk(const std::string& InChunkPath);
        void LoadLevel(const std::string& InPath);
        void UnloadAllLevels();
        void RegisterTickTask(TickTask* InTask);
        void UnregisterTickTask(TickTask* InTask);
        const Vector3& GetWorldOrigin() const { return mWorldOrigin; }
        Level* LoadLevelAdditive(const std::string& InLevelPath);
        GameObject* CreateGameObject();
        GameObjectMgr* GetGameObjectMgr() const { return mGameObjectMgr; };
        SceneManager* GetSceneManager() const { return mSceneManager; }
    private:
        void AddToWorld(GameObject* InObject);
    private:
        std::vector<Level*> mLevels;
        std::vector<GameObject*> mActiveGameObjects;
        LevelManager* mLevelManager;
        TickManager* mTickManager;
        GameObjectMgr* mGameObjectMgr;
        SceneManager* mSceneManager = nullptr;

        // loaded chunk map: chunk path -> Level*
        std::map<std::string, Level*> mLoadedChunks;
        Vector3 mWorldOrigin = Vector3::ZERO;
    };
}