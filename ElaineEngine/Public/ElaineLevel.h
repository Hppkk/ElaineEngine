#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineVector3.h"

namespace Elaine
{
    class World;

    class ElaineEngineExport Level
    {
    public:
        Level(World* InWorld);
        ~Level();
        void SetWorldOffset(const Vector3& InOffset);
        const Vector3& GetWorldOrigin() const { return mWorldOrigin; }
        bool Load(const std::string& InPath);
        void OnLevelLoaded(World* InWorld);
        void OnLevelUnloaded();
        const std::vector<GameObject*>& GetGameObjects() const { return mGameObjects; }

    private:
        std::vector<GameObject*> mGameObjects;
        std::string mName;
        Vector3 mWorldOffset; //相对于世界的偏移
        Vector3 mWorldOrigin; //世界空间下的原点
        World* mWorld;
    };
}