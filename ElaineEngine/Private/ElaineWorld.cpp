#include "ElainePrecompiledHeader.h"
#include "ElaineWorld.h"
#include "ElaineWorldChunk.h"
#include "GamePlay/ElaineActor.h"
#include "GamePlay/ElaineActorManager.h"
#include "ElaineTickManager.h"
#include "ElaineDataStream.h"
#include "ElaineRenderCommandQueue.h"
#include "math/ElaineDynamicBVH.h"
#include "math/ElaineISpatialObject.h"

namespace Elaine
{
    namespace
    {
        bool ReadVector3(const JsonCpp& InNode, Vector3& OutValue)
        {
            if (!InNode.is_array() || InNode.size() < 3)
                return false;
            OutValue = Vector3(InNode[0].get<float>(), InNode[1].get<float>(), InNode[2].get<float>());
            return true;
        }

        bool ReadJsonFile(const std::string& InPath, JsonCpp& OutJson)
        {
            DataStream Stream(Root::instance()->GetResourcePath() + InPath, DataStream::In);
            Stream.ReadAll();
            if (Stream.GetDataStream() == nullptr)
                return false;
            OutJson = JsonCpp(Stream.GetDataStream());
            return !OutJson.is_null();
        }
    }

    World::World()
    {
        mTickManager = new TickManager();
        mActorManager = new ActorManager(this);
        mSceneBVH = new DynamicBVH();
        ENQUEUE_RENDER_COMMAND(CreateSceneManager)([this](RenderContext& Context)
        {
            mSceneManager = Root::instance()->CreateSceneManager("Main SceneManager");
        });
    }

    World::~World()
    {
        for (auto& Entry : mLoadedChunks)
        {
            if (Entry.second)
                Entry.second->Unload(this);
            SAFE_DELETE(Entry.second);
        }
        mLoadedChunks.clear();
        mActiveActors.clear();
        ENQUEUE_RENDER_COMMAND(DestroySceneManager)([this](RenderContext& Context)
        {
            if (mSceneManager)
                Root::instance()->DestroySceneManager(mSceneManager);
        });
        mSceneManager = nullptr;
        SAFE_DELETE(mSceneBVH);
        SAFE_DELETE(mActorManager);
        SAFE_DELETE(mTickManager);
    }

    void World::Tick(float InDeltaTime)
    {
        UpdateChunkStreaming(InDeltaTime);
        mTickManager->RunTickGroup(TickGroup::FixedUpdate, InDeltaTime);
        mTickManager->RunTickGroup(TickGroup::Update, InDeltaTime);
        mTickManager->RunTickGroup(TickGroup::LateUpdate, InDeltaTime);
    }

    Actor* World::CreateActor()
    {
        Actor* NewActor = mActorManager->CreateActor();
        NewActor->Initialize();
        AddToWorld(NewActor);
        return NewActor;
    }

    StreamingObserverId World::RegisterStreamingObserver(const Vector3& InPosition)
    {
        const StreamingObserverId NewId = mNextObserverId++;
        mStreamingObservers.emplace(NewId, InPosition);
        return NewId;
    }

    void World::UpdateStreamingObserver(StreamingObserverId InId, const Vector3& InPosition)
    {
        auto It = mStreamingObservers.find(InId);
        if (It != mStreamingObservers.end())
            It->second = InPosition;
    }

    void World::UnregisterStreamingObserver(StreamingObserverId InId)
    {
        mStreamingObservers.erase(InId);
    }

    bool World::LoadChunk(const std::string& InChunkId)
    {
        auto RecordIt = mChunkRecords.find(InChunkId);
        if (RecordIt == mChunkRecords.end())
            return false;
        if (mLoadedChunks.find(InChunkId) != mLoadedChunks.end())
            return true;
        WorldChunk* NewChunk = new WorldChunk(RecordIt->second);
        if (!NewChunk->Load(this))
        {
            SAFE_DELETE(NewChunk);
            return false;
        }
        mLoadedChunks.emplace(InChunkId, NewChunk);
        return true;
    }

    void World::UnloadChunk(const std::string& InChunkId)
    {
        auto It = mLoadedChunks.find(InChunkId);
        if (It == mLoadedChunks.end())
            return;
        if (It->second)
            It->second->Unload(this);
        SAFE_DELETE(It->second);
        mLoadedChunks.erase(It);
    }

    bool World::IsChunkWanted(const WorldChunkRecord& InRecord) const
    {
        if (mStreamingObservers.empty())
            return false;
        const Vector3 Size = InRecord.Bounds.getSize();
        const Vector3 Min = InRecord.Bounds.getMin() - Size * static_cast<float>(mLoadRadius);
        const Vector3 Max = InRecord.Bounds.getMax() + Size * static_cast<float>(mLoadRadius);
        for (const auto& Observer : mStreamingObservers)
        {
            const Vector3& Position = Observer.second;
            if (Position.x >= Min.x && Position.x <= Max.x && Position.y >= Min.y && Position.y <= Max.y && Position.z >= Min.z && Position.z <= Max.z)
                return true;
        }
        return false;
    }

    void World::UpdateChunkStreaming(float InDeltaTime)
    {
        std::vector<std::string> ChunksToLoad;
        for (const auto& Entry : mChunkRecords)
            if (Entry.second.LoadOnStart || IsChunkWanted(Entry.second))
                ChunksToLoad.push_back(Entry.first);
        for (const std::string& ChunkId : ChunksToLoad)
            LoadChunk(ChunkId);

        if (mStreamingObservers.empty())
            return;
        for (auto It = mLoadedChunks.begin(); It != mLoadedChunks.end(); )
        {
            const WorldChunkRecord& Record = mChunkRecords[It->first];
            const Vector3 Size = Record.Bounds.getSize();
            const Vector3 Min = Record.Bounds.getMin() - Size * static_cast<float>(mUnloadRadius);
            const Vector3 Max = Record.Bounds.getMax() + Size * static_cast<float>(mUnloadRadius);
            bool KeepLoaded = false;
            for (const auto& Observer : mStreamingObservers)
            {
                const Vector3& Position = Observer.second;
                if (Position.x >= Min.x && Position.x <= Max.x && Position.y >= Min.y && Position.y <= Max.y && Position.z >= Min.z && Position.z <= Max.z)
                {
                    KeepLoaded = true;
                    break;
                }
            }
            const std::string ChunkId = It->first;
            ++It;
            if (!KeepLoaded && !mChunkRecords[ChunkId].LoadOnStart)
                UnloadChunk(ChunkId);
        }
    }

    bool World::SaveWorld(const std::string& InPath)
    {
        JsonCpp JsonData;
        JsonData["Version"] = 1;
        JsonData["Origin"] = JsonCpp::array({ mWorldOrigin.x, mWorldOrigin.y, mWorldOrigin.z });
        JsonData["Streaming"]["LoadRadius"] = mLoadRadius;
        JsonData["Streaming"]["UnloadRadius"] = mUnloadRadius;
        JsonData["Chunks"] = JsonCpp::array();
        for (const auto& Entry : mChunkRecords)
        {
            const WorldChunkRecord& Record = Entry.second;
            JsonCpp Chunk;
            Chunk["Id"] = Record.Id;
            Chunk["Coord"] = JsonCpp::array({ Record.X, Record.Y });
            Chunk["Bounds"]["Min"] = JsonCpp::array({ Record.Bounds.getMin().x, Record.Bounds.getMin().y, Record.Bounds.getMin().z });
            Chunk["Bounds"]["Max"] = JsonCpp::array({ Record.Bounds.getMax().x, Record.Bounds.getMax().y, Record.Bounds.getMax().z });
            Chunk["Actors"] = JsonCpp::array();
            for (const std::string& ActorPath : Record.ActorPaths)
                Chunk["Actors"].push_back(ActorPath);
            Chunk["Bundle"] = Record.BundlePath;
            Chunk["LoadOnStart"] = Record.LoadOnStart;
            JsonData["Chunks"].push_back(Chunk);
        }
        JsonData["LoadedChunks"] = JsonCpp::array();
        for (const auto& Entry : mLoadedChunks)
            JsonData["LoadedChunks"].push_back(Entry.first);
        const std::string FullPath = Root::instance()->GetResourcePath() + InPath;
        DataStream Out(FullPath, DataStream::Out);
        const std::string Text = JsonData.dump(4);
        Out.Write(Text.data(), Text.size());
        return true;
    }

    bool World::LoadWorld(const std::string& InPath)
    {
        JsonCpp JsonData;
        if (!ReadJsonFile(InPath, JsonData))
            return false;
        for (auto It = mLoadedChunks.begin(); It != mLoadedChunks.end(); )
        {
            const std::string ChunkId = It->first;
            ++It;
            UnloadChunk(ChunkId);
        }
        if (JsonData.contains("Origin"))
            ReadVector3(JsonData["Origin"], mWorldOrigin);
        if (JsonData.contains("Streaming"))
        {
            mLoadRadius = JsonData["Streaming"].value("LoadRadius", mLoadRadius);
            mUnloadRadius = JsonData["Streaming"].value("UnloadRadius", mUnloadRadius);
        }
        mChunkRecords.clear();
        if (JsonData.contains("Chunks") && JsonData["Chunks"].is_array())
        {
            for (const JsonCpp& ChunkNode : JsonData["Chunks"])
            {
                WorldChunkRecord Record;
                Record.Id = ChunkNode.value("Id", std::string());
                const JsonCpp Coord = ChunkNode.value("Coord", JsonCpp::array());
                if (Coord.is_array() && Coord.size() >= 2)
                {
                    Record.X = Coord[0].get<int32_t>();
                    Record.Y = Coord[1].get<int32_t>();
                }
                if (ChunkNode.contains("Bounds"))
                {
                    Vector3 Min;
                    Vector3 Max;
                    if (ReadVector3(ChunkNode["Bounds"].value("Min", JsonCpp::array()), Min) && ReadVector3(ChunkNode["Bounds"].value("Max", JsonCpp::array()), Max))
                        Record.Bounds.setExtent(Min, Max);
                }
                Record.BundlePath = ChunkNode.value("Bundle", std::string());
                Record.LoadOnStart = ChunkNode.value("LoadOnStart", false);
                if (ChunkNode.contains("Actors") && ChunkNode["Actors"].is_array())
                    for (const JsonCpp& ActorPath : ChunkNode["Actors"])
                        if (ActorPath.is_string())
                            Record.ActorPaths.push_back(ActorPath.get<std::string>());
                if (!Record.Id.empty())
                    mChunkRecords[Record.Id] = Record;
            }
        }
        if (JsonData.contains("LoadedChunks") && JsonData["LoadedChunks"].is_array())
            for (const JsonCpp& ChunkId : JsonData["LoadedChunks"])
                if (ChunkId.is_string())
                    LoadChunk(ChunkId.get<std::string>());
        for (const auto& Entry : mChunkRecords)
            if (Entry.second.LoadOnStart)
                LoadChunk(Entry.first);
        return true;
    }

    void World::RegisterTickTask(TickTask* InTask) { mTickManager->RegisterTickTask(InTask); }
    void World::UnregisterTickTask(TickTask* InTask) { mTickManager->UnregisterTickTask(InTask); }

    void World::AddToWorld(Actor* InObject)
    {
        if (InObject == nullptr)
            return;
        InObject->OnRegisterWorld(this);
        mActiveActors.push_back(InObject);
    }

    void World::RemoveFromWorld(Actor* InObject)
    {
        if (InObject == nullptr)
            return;
        auto It = std::find(mActiveActors.begin(), mActiveActors.end(), InObject);
        if (It != mActiveActors.end())
            mActiveActors.erase(It);
        InObject->OnUnregisterWorld();
    }

    ISpatialObject* World::Raycast(const Ray& InRay, float MaxDistance) const
    {
        if (mSceneBVH)
            return mSceneBVH->Raycast(InRay, MaxDistance).Object;
        return nullptr;
    }

    std::vector<ISpatialObject*> World::BoxIntersect(const AxisAlignedBox& InBox) const
    {
        return mSceneBVH ? mSceneBVH->BoxIntersect(InBox) : std::vector<ISpatialObject*>();
    }
}
