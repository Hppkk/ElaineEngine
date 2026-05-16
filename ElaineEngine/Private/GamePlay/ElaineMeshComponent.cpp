#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineMeshComponent.h"
#include "ElaineRenderContext.h"
#include "RenderProxy/ElaineMeshRenderProxy.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineMeshManager.h"
#include "ElaineMaterialInstanceDynamic.h"
#include "ElaineMaterialParamSnapshot.h"
#include "ElaineGameObject.h"

namespace Elaine
{
    StaticMeshComponent::StaticMeshComponent(GameObject* InObject)
        : Component(InObject)
    {

    }

    StaticMeshComponent::~StaticMeshComponent()
    {
        // 逻辑线程销毁材质实例
        for (auto* Mat : mMaterials)
        {
            delete Mat;
        }
        mMaterials.clear();
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
        // 在逻辑线程生成所有材质的快照
        std::vector<MaterialParamSnapshot> Snapshots;
        Snapshots.reserve(mMaterials.size());
        for (const auto* Mat : mMaterials)
        {
            if (Mat)
                Snapshots.push_back(Mat->CreateSnapshot());
            else
                Snapshots.push_back(MaterialParamSnapshot());
        }

        MeshPtr CurrentMesh = mMesh;
        uint32 Count = 0;
        if (!CurrentMesh.isNull())
            Count = CurrentMesh->GetSubMeshCount();

        ENQUEUE_RENDER_COMMAND(CreateProxy)(
            [this, Snapshots = std::move(Snapshots), CurrentMesh, Count](RenderContext& Context)
            {
                mRenderProxy = static_cast<StaticMeshRenderProxy*>(GetGameObject()->GetSceneManager()->CreateRenderProxy(EProxyType::StaticMesh));
                if (mRenderProxy)
                {
                    mRenderProxy->SetMesh(CurrentMesh);
                    // 用快照数组更新渲染线程的 RenderMaterialProxy（不再传递 MaterialInstanceDynamic*）
                    mRenderProxy->UpdateMaterials(Snapshots);
                    mRenderProxy->SetSubMeshCount(Count);
                    mRenderProxy->SetCastShadow(mbCastShadow);
                    mRenderProxy->SetReceiveShadow(mbReceiveShadow);
                    mRenderProxy->SetRenderLayer(mRenderLayer);
                    mRenderProxy->mUserData = GetGameObject();
                    mRenderProxy->mUserType = 1; // 1 = GameObject
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

        // 在逻辑线程生成所有材质的快照
        std::vector<MaterialParamSnapshot> Snapshots;
        Snapshots.reserve(mMaterials.size());
        for (const auto* Mat : mMaterials)
        {
            if (Mat)
                Snapshots.push_back(Mat->CreateSnapshot());
            else
                Snapshots.push_back(MaterialParamSnapshot());
        }

        StaticMeshRenderProxy* Proxy = mRenderProxy;
        MeshPtr CurrentMesh = mMesh;
        uint32 Count = 0;
        if (!CurrentMesh.isNull())
            Count = CurrentMesh->GetSubMeshCount();

        ENQUEUE_RENDER_COMMAND(UpdateMeshState)(
            [Proxy, CurrentMesh, Snapshots = std::move(Snapshots), Count, Cast = mbCastShadow, Recv = mbReceiveShadow, Layer = mRenderLayer](RenderContext& Context)
            {
                Proxy->SetMesh(CurrentMesh);
                Proxy->SetSubMeshCount(Count);
                // 用快照数组更新渲染线程的 RenderMaterialProxy
                Proxy->UpdateMaterials(Snapshots);
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