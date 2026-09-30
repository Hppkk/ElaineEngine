#pragma once

#include "ElaineEnginePrerequirements.h"
#include "ElaineVector3.h"
#include "math/ElaineAxisAlignedBox.h"

namespace Elaine
{
    class Actor;
    class World;

    enum class WorldChunkState : uint8_t
    {
        Unloaded,
        Loading,
        Loaded,
        Unloading,
        Failed
    };

    struct ElaineEngineExport WorldChunkRecord
    {
        std::string Id;
        int32_t X = 0;
        int32_t Y = 0;
        AxisAlignedBox Bounds;
        std::vector<std::string> ActorPaths;
        std::string BundlePath;
        bool LoadOnStart = false;
    };

    class ElaineEngineExport WorldChunk
    {
    public:
        explicit WorldChunk(const WorldChunkRecord& InRecord);

        const WorldChunkRecord& GetRecord() const { return mRecord; }
        WorldChunkState GetState() const { return mState; }
        const std::vector<Actor*>& GetActors() const { return mActors; }

    private:
        bool Load(World* InWorld);
        void Unload(World* InWorld);

        WorldChunkRecord mRecord;
        WorldChunkState mState = WorldChunkState::Unloaded;
        std::vector<Actor*> mActors;

        friend class World;
    };
}
