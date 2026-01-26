#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "Resource/ElaineTextureResource.h"
#include "ElaineMaterial.h"

namespace Elaine
{
    class MaterialInstanceDynamic;

    class ElaineCoreExport SkyRenderProxy : public RenderProxy
    {
    public:
        SkyRenderProxy();
        virtual ~SkyRenderProxy();

        void SetCubeTexture(const TexturePtr& InCube) { mCubeTexture = InCube; }
        void SetExposure(float InExposure) { mExposure = InExposure; }
        void SetRotation(const Quaternion& InRot) { mRotation = InRot; }
        void SetMaterial(MaterialInstanceDynamic* InMaterial) { mMaterial = InMaterial; }

        TexturePtr GetCubeTexture() const { return mCubeTexture; }
        float GetExposure() const { return mExposure; }
        const Quaternion& GetRotation() const { return mRotation; }
        MaterialInstanceDynamic* GetMaterial() const { return mMaterial; }

        // ========== 重写基类虚函数 ==========
        void UpdateRenderQueue(RenderQueueSet* InRenderQueue) override;
        void PrepareResourceBinding() override;
        void InitializeResourceBinding() override;

    private:
        TexturePtr mCubeTexture;
        float mExposure = 1.0f;
        Quaternion mRotation = Quaternion::IDENTITY;
        MaterialInstanceDynamic* mMaterial = nullptr;
    };

}


