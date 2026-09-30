#include "ElainePrecompiledHeader.h"
#include "ElaineWorldChunk.h"
#include "ElaineWorld.h"
#include "GamePlay/ElaineActor.h"
#include "GamePlay/ElaineActorManager.h"
#include "ElaineDataStream.h"

namespace Elaine
{
    WorldChunk::WorldChunk(const WorldChunkRecord& InRecord)
        : mRecord(InRecord)
    {
    }

    bool WorldChunk::Load(World* InWorld)
    {
        if (InWorld == nullptr || mState == WorldChunkState::Loaded)
            return mState == WorldChunkState::Loaded;

        mState = WorldChunkState::Loading;
        std::vector<std::string> ActorPaths = mRecord.ActorPaths;
        if (ActorPaths.empty() && !mRecord.BundlePath.empty())
        {
            DataStream Bundle(Root::instance()->GetResourcePath() + mRecord.BundlePath, DataStream::In);
            Bundle.ReadAll();
            if (Bundle.GetDataStream())
            {
                JsonCpp BundleJson(Bundle.GetDataStream());
                if (BundleJson.contains("Actors") && BundleJson["Actors"].is_array())
                    for (const JsonCpp& ActorPath : BundleJson["Actors"])
                        if (ActorPath.is_string())
                            ActorPaths.push_back(ActorPath.get<std::string>());
            }
        }
        for (const std::string& ActorPath : ActorPaths)
        {
            Actor* NewActor = InWorld->GetActorManager()->CreateActorByInfo(ActorPath, false);
            if (NewActor == nullptr)
            {
                mState = WorldChunkState::Failed;
                Unload(InWorld);
                return false;
            }

            mActors.push_back(NewActor);
            InWorld->AddToWorld(NewActor);
        }

        mState = WorldChunkState::Loaded;
        return true;
    }

    void WorldChunk::Unload(World* InWorld)
    {
        if (InWorld == nullptr)
            return;

        mState = WorldChunkState::Unloading;
        for (Actor* CurrentActor : mActors)
        {
            if (CurrentActor == nullptr)
                continue;
            InWorld->GetActorManager()->DestroyActor(CurrentActor);
        }
        mActors.clear();
        mState = WorldChunkState::Unloaded;
    }
}
