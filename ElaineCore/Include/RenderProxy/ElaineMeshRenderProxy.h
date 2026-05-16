#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "RenderProxy/ElaineRenderMaterialProxy.h"
#include "ElaineMesh.h"

namespace Elaine
{
    class ElaineCoreExport StaticMeshRenderProxy : public RenderProxy
    {
    public:
        StaticMeshRenderProxy();
        virtual ~StaticMeshRenderProxy();
        void SetMesh(const MeshPtr& InMesh) { mMesh = InMesh; }
        void SetSubMeshCount(uint32 InCount) { mSubMeshCount = InCount; }
        void SetCastShadow(bool b) { mbCastShadow = b; }
        void SetReceiveShadow(bool b) { mbReceiveShadow = b; }
        void SetRenderLayer(uint8 L) { mRenderLayer = L; }

        /**
         * 从逻辑线程的 MaterialParamSnapshot 数组更新渲染线程的材质代理。
         * 必须在渲染线程（ENQUEUE_RENDER_COMMAND 的 lambda 中）调用。
         */
        void UpdateMaterials(const std::vector<MaterialParamSnapshot>& InSnapshots);

        /** 获取指定索引的渲染线程材质代理（仅渲染线程访问） */
        RenderMaterialProxy* GetMaterialProxy(uint32 InIndex);
        const RenderMaterialProxy* GetMaterialProxy(uint32 InIndex) const;

        void UpdateRenderQueue(RenderQueueSet* InRenderQueue) override;

    private:
        uint32 mSubMeshCount;
        bool mbCastShadow = true;
        bool mbReceiveShadow = true;
        uint8 mRenderLayer = 0;
        MeshPtr mMesh;
        std::vector<RenderMaterialProxy> mMaterialProxies;  // 渲染线程材质代理数组（值语义）
    };
}