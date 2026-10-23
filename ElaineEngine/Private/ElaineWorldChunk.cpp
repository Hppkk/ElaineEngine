#include "ElainePrecompiledHeader.h"
#include "ElaineWorldChunk.h"
#include "ElaineWorld.h"
#include "GamePlay/ElaineActor.h"
#include "GamePlay/ElaineActorManager.h"
#include "ElaineDataStream.h"
#include "ElaineActorInfoMgr.h"
#include "Resource/ElaineResourceRegistry.h"

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
        std::vector<ResourceReference> ActorReferences = mRecord.ActorReferences;
        if (ActorReferences.empty() && mRecord.Bundle.Id.IsValid())
        {
            const auto BundlePath = ResourceRegistry::Get().Resolve(mRecord.Bundle);
            {
                JsonCpp BundleJson = AssetFile::ReadJson(std::filesystem::path(Root::instance()->GetResourcePath()) / BundlePath, "ActorBundle");
                if (BundleJson.contains("Actors") && BundleJson["Actors"].is_array())
                    for (const JsonCpp& ActorPath : BundleJson["Actors"])
                        ActorReferences.push_back(ResourceReference::FromJson(ActorPath));
            }
        }
        // A chunk is instantiated only after all actor descriptions have published.
        bool Waiting = false;
        for (const auto& Path : ActorReferences)
        {
            auto Info = ActorInfoMgr::instance()->GetResource(Path);
            if (!Info || Info->GetLoadingState() == ResourceLoadState::Failed || Info->GetLoadingState() == ResourceLoadState::Cancelled)
            { mState = WorldChunkState::Failed; return false; }
            if (!Info->IsReady()) Waiting = true;
        }
        if (Waiting) return false;
        for (const auto& ActorPath : ActorReferences)
        {
            Actor* NewActor = InWorld->GetActorManager()->CreateActorByInfo(ActorInfoMgr::instance()->GetResource(ActorPath));
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
