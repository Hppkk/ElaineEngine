#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineVector3.h"
#include "ElaineWorldChunk.h"

namespace Elaine
{
    class Actor;
    class TickManager;
    struct TickTask;
    class ActorManager;
    class SceneManager;
    class DynamicBVH;
    class Ray;
    class AxisAlignedBox;
    class ISpatialObject;
    using StreamingObserverId = uint32_t;

    class ElaineEngineExport World
    {
    public:
        World();
        ~World();
        void Tick(float InDeltaTime);
        // World-level persist/load
        bool SaveWorld(const std::string& InPath);
        bool LoadWorld(const std::string& InPath);
        bool LoadChunk(const std::string& InChunkId);
        void UnloadChunk(const std::string& InChunkId);
        StreamingObserverId RegisterStreamingObserver(const Vector3& InPosition);
        void UpdateStreamingObserver(StreamingObserverId InId, const Vector3& InPosition);
        void UnregisterStreamingObserver(StreamingObserverId InId);
        void UpdateChunkStreaming(float InDeltaTime);
        void RegisterTickTask(TickTask* InTask);
        void UnregisterTickTask(TickTask* InTask);
        const Vector3& GetWorldOrigin() const { return mWorldOrigin; }
        Actor* CreateActor();
        ActorManager* GetActorManager() const { return mActorManager; };
        SceneManager* GetSceneManager() const { return mSceneManager; }
        const std::vector<Actor*>& GetActors() const { return mActiveActors; }

        DynamicBVH* GetSceneBVH() const { return mSceneBVH; }
        ISpatialObject* Raycast(const Ray& InRay, float MaxDistance = 3.402823466e+38F) const;
        std::vector<ISpatialObject*> BoxIntersect(const AxisAlignedBox& InBox) const;

    private:
        void AddToWorld(Actor* InObject);
        void RemoveFromWorld(Actor* InObject);
        bool IsChunkWanted(const WorldChunkRecord& InRecord) const;
        friend class WorldChunk;
        friend class ActorManager;
    private:
        std::vector<Actor*> mActiveActors;
        TickManager* mTickManager;
        ActorManager* mActorManager;
        SceneManager* mSceneManager = nullptr;
        DynamicBVH* mSceneBVH = nullptr;

        std::map<std::string, WorldChunkRecord> mChunkRecords;
        std::map<std::string, WorldChunk*> mLoadedChunks;
        std::map<StreamingObserverId, Vector3> mStreamingObservers;
        StreamingObserverId mNextObserverId = 1;
        int mLoadRadius = 1;
        int mUnloadRadius = 2;
        Vector3 mWorldOrigin = Vector3::ZERO;
    };
}
