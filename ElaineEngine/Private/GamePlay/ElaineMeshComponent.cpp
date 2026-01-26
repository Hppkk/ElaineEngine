#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineMeshComponent.h"
#include "ElaineRenderContext.h"
#include "RenderProxy/ElaineMeshRenderProxy.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineMeshManager.h"
#include "ElaineMaterialInstanceDynamic.h"
#include "ElaineGameObject.h"

namespace Elaine
{
    StaticMeshComponent::StaticMeshComponent(GameObject* InObject)
        : Component(InObject)
    {

    }

    StaticMeshComponent::~StaticMeshComponent()
    {

    }

    void StaticMeshComponent::ChangeMesh(const std::string& InPath)
    {
        if (InPath == mPath)
            return;

        mMesh = MeshManager::instance()->GetResource(InPath);
        MarkRenderStateDirty();
    }

    void StaticMeshComponent::ChangeMaterial(uint32 InIndex, const std::string& InMatName)
    {
        if (InIndex >= mMaterials.size())
            return;

        mMaterials[InIndex] = new MaterialInstanceDynamic();
        mMaterials[InIndex]->ChangeMaterial(InMatName);
        MarkRenderStateDirty();
    }

    void StaticMeshComponent::OnRegisterWorldImpl(World* InWorld)
    {
        ENQUEUE_RENDER_COMMAND(CreateProxy)(
            [this](RenderContext& Context)
            {
                mRenderProxy = static_cast<StaticMeshRenderProxy*>(GetGameObject()->GetSceneManager()->CreateRenderProxy(EProxyType::StaticMesh));
                // initialize with current mesh/material state
                if (mRenderProxy)
                {
                    mRenderProxy->SetMesh(mMesh);
                    mRenderProxy->SetMaterials(mMaterials);
                    // submesh count could be derived from mesh if available
                    uint32 count = 0;
                    if (!mMesh.isNull())
                        count = mMesh->GetSubMeshCount();
                    mRenderProxy->SetSubMeshCount(count);
                    mRenderProxy->SetCastShadow(mbCastShadow);
                    mRenderProxy->SetReceiveShadow(mbReceiveShadow);
                    mRenderProxy->SetRenderLayer(mRenderLayer);
                }
            });
    }

    void StaticMeshComponent::OnUnregisterWorldImpl()
    {
        if (mRenderProxy)
        {
            StaticMeshRenderProxy* ProxyToDestroy = mRenderProxy;
            mRenderProxy = nullptr;

            ENQUEUE_RENDER_COMMAND(DestroyProxy)(
                [ProxyToDestroy, this](RenderContext& Context)
                {
                    GetGameObject()->GetSceneManager()->DestroyRenderProxy(ProxyToDestroy);
                });
        }
    }

    void StaticMeshComponent::MarkRenderStateDirty()
    {
        if (mRenderProxy == nullptr)
            return;

        StaticMeshRenderProxy* Proxy = mRenderProxy;
        MeshPtr CurrentMesh = mMesh;
        std::vector<MaterialInstanceDynamic*> Mats = mMaterials;
        uint32 Count = 0;
        if (!CurrentMesh.isNull())
            Count = CurrentMesh->GetSubMeshCount();

        ENQUEUE_RENDER_COMMAND(UpdateMeshState)(
            [Proxy, CurrentMesh, Mats, Count, Cast = mbCastShadow, Recv = mbReceiveShadow, Layer = mRenderLayer](RenderContext& Context)
            {
                Proxy->SetMesh(CurrentMesh);
                Proxy->SetSubMeshCount(Count);
                Proxy->SetMaterials(Mats);
                Proxy->SetCastShadow(Cast);
                Proxy->SetReceiveShadow(Recv);
                Proxy->SetRenderLayer(Layer);
            });
    }

    void StaticMeshComponent::MarkTransformDirty()
    {
        if (mRenderProxy == nullptr)
            return;

        StaticMeshRenderProxy* Proxy = mRenderProxy;
        Matrix4x4 WorldMat = GetGameObject()->GetWorldMatrix();
        Vector3 WorldPos = GetGameObject()->GetWorldPosition();
        Vector3 WorldScale = GetGameObject()->GetWorldScale();

        ENQUEUE_RENDER_COMMAND(UpdateProxy)(
            [=](RenderContext& Context)
            {
                Proxy->SetWorldMatrix(WorldMat);
                Proxy->SetWorldPosition(WorldPos);
                Proxy->SetWorldScale(WorldScale);
            });
    }

    const Name& StaticMeshComponent::GetType() const
    {
        static Name NameType("StaticMeshComponent");
        return NameType;
    }
}