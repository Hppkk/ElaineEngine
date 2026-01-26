#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "ElaineMesh.h"
#include "ElaineMaterialInstanceDynamic.h"

namespace Elaine
{
    class ElaineCoreExport StaticMeshRenderProxy : public RenderProxy
    {
    public:
        StaticMeshRenderProxy();
        virtual ~StaticMeshRenderProxy();
        void SetMesh(const MeshPtr& InMesh) { mMesh = InMesh; }
        void SetMaterials(const std::vector<MaterialInstanceDynamic*>& InMats) { mMaterials = InMats; }
        void SetSubMeshCount(uint32 InCount) { mSubMeshCount = InCount; }
        void SetCastShadow(bool b) { mbCastShadow = b; }
        void SetReceiveShadow(bool b) { mbReceiveShadow = b; }
        void SetRenderLayer(uint8 L) { mRenderLayer = L; }
        void UpdateRenderQueue(RenderQueueSet* InRenderQueue) override;
    private:
        uint32 mSubMeshCount;
        bool mbCastShadow = true;
        bool mbReceiveShadow = true;
        uint8 mRenderLayer = 0;
        MeshPtr mMesh;
        std::vector<MaterialInstanceDynamic*> mMaterials;
    };
}