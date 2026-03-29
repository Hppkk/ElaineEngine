#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "ElaineMaterial.h"

namespace Elaine
{
    class MaterialInstanceDynamic;

    class ElaineCoreExport GridRenderProxy : public RenderProxy
    {
    public:
        GridRenderProxy();
        virtual ~GridRenderProxy();

        void SetMaterial(MaterialInstanceDynamic* InMaterial) { mMaterial = InMaterial; }
        MaterialInstanceDynamic* GetMaterial() const { return mMaterial; }

        // ========== 重写基类虚函数 ==========
        void UpdateRenderQueue(RenderQueueSet* InRenderQueue) override;
        void PrepareResourceBinding() override;
        void InitializeResourceBinding() override;

    private:
        MaterialInstanceDynamic* mMaterial = nullptr;
    };
}
