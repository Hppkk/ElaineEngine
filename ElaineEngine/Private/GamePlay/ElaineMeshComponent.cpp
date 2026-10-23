#include "ElainePrecompiledHeader.h"
#include "GamePlay/ElaineMeshComponent.h"
#include "ElaineRenderContext.h"
#include "RenderProxy/ElaineMeshRenderProxy.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineMeshManager.h"
#include "ElaineMaterialInstanceDynamic.h"
#include "ElaineMaterialParamSnapshot.h"
#include "ElaineActor.h"
#include "ElaineWorld.h"
#include "math/ElaineDynamicBVH.h"

namespace Elaine
{
    StaticMeshComponent::StaticMeshComponent(Actor* InObject)
        : ActorComponent(InObject)
    {

    }

    StaticMeshComponent::~StaticMeshComponent()
    {
        // 在销毁组件和材质前解除通知。
        mMaterialSubscriptions.clear();
        mMeshSubscription.Reset();
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

        mPath = InPath;
        ApplyMeshPath();
    }

    void StaticMeshComponent::ApplyMeshPath()
    {
        mMeshSubscription.Reset();
        mMesh = mPath.empty() ? MeshPtr() : MeshManager::instance()->GetResource(mPath);
        if (mMesh) mMeshSubscription = mMesh->ObserveChanges([this](const ResourceLoadResult&) { MarkRenderStateDirty(); MarkTransformDirty(); });
        MarkRenderStateDirty();
        MarkTransformDirty();
    }
    void StaticMeshComponent::ApplyVisibility() { MarkRenderStateDirty(); }
    void StaticMeshComponent::PostEditChangeProperty(const PropertyChangedEvent& Event)
    {
        ActorComponent::PostEditChangeProperty(Event);
        if (Event.Type == PropertyChangeType::EditFinished) return;
        if (Event.IsProperty("mPath")) ApplyMeshPath();
        else if (Event.IsProperty("mbCastShadow") || Event.IsProperty("mbReceiveShadow") || Event.IsProperty("mRenderLayer"))
            MarkRenderStateDirty();
    }

    void StaticMeshComponent::ChangeMaterial(uint32 InIndex, const std::string& InMatName)
    {
        if (InIndex >= mMaterials.size())
        {
            mMaterials.resize(static_cast<size_t>(InIndex) + 1, nullptr);
            mMaterialSubscriptions.resize(mMaterials.size());
        }

        mMaterialSubscriptions[InIndex].Reset();
        delete mMaterials[InIndex];
        mMaterials[InIndex] = nullptr;
        if (InMatName.empty()) { MarkRenderStateDirty(); return; }
        mMaterials[InIndex] = new MaterialInstanceDynamic();
        mMaterials[InIndex]->ChangeMaterial(InMatName);
        mMaterialSubscriptions[InIndex] = mMaterials[InIndex]->ObserveParameterChanges([this] {
            MarkRenderStateDirty();
        });
        MarkRenderStateDirty();
    }

    std::vector<MaterialParamSnapshot> StaticMeshComponent::CaptureMaterials() const
    {
        const size_t SlotCount = mMesh && mMesh->IsReady() ? mMesh->GetMaterialSlots().size() : 0;
        std::vector<MaterialParamSnapshot> Snapshots;
        for (size_t I=0; I<std::max(SlotCount, mMaterials.size()); ++I)
        {
            if (I < mMaterials.size() && mMaterials[I]) Snapshots.push_back(mMaterials[I]->CreateSnapshot());
            else
            {
                MaterialParamSnapshot Snapshot;
                if (I < SlotCount) Snapshot.Source = mMesh->GetDefaultMaterial(static_cast<uint32>(I));
                if (Snapshot.Source) { Snapshot.Revision = Snapshot.Source->GetContentRevision(); Snapshot.CaptureResources(); }
                Snapshots.push_back(std::move(Snapshot));
            }
        }
        return Snapshots;
    }

    void StaticMeshComponent::OnRegisterWorldImpl(World* InWorld)
    {
        // 在逻辑线程生成所有材质的快照
        auto Snapshots = CaptureMaterials();

        MeshPtr CurrentMesh = mMesh;
        const auto MeshSnapshot = CurrentMesh && CurrentMesh->IsReady() ? CurrentMesh->CreateRenderData() : MeshRenderData{};
        uint32 Count = 0;
        if (!CurrentMesh.isNull())
            Count = CurrentMesh->GetSubMeshCount();

        auto Result = std::make_shared<StaticMeshRenderProxy*>(nullptr);
        auto* Scene = GetActor()->GetSceneManager(); auto* ActorIdentity = GetActor();
        ENQUEUE_RENDER_COMMAND(CreateProxy)(
            [Result, Scene, ActorIdentity, Snapshots = std::move(Snapshots), CurrentMesh, MeshSnapshot, Count,
                Cast = mbCastShadow, Recv = mbReceiveShadow, Layer = mRenderLayer, Visible = mbVisible](RenderContext& Context)
            {
                auto* Proxy = static_cast<StaticMeshRenderProxy*>(Scene->CreateRenderProxy(EProxyType::StaticMesh));
                *Result = Proxy;
                if (Proxy)
                {
                    Proxy->SetMesh(CurrentMesh, MeshSnapshot);
                    // 用快照数组更新渲染线程的 RenderMaterialProxy（不再传递 MaterialInstanceDynamic*）
                    Proxy->UpdateMaterials(Snapshots, Context);
                    Proxy->SetSubMeshCount(Count);
                    Proxy->SetCastShadow(Cast);
                    Proxy->SetReceiveShadow(Recv);
                    Proxy->SetRenderLayer(Layer);
                    Proxy->SetVisible(Visible);
                    Proxy->mUserData = ActorIdentity;
                    Proxy->mUserType = 1; // 1 = Actor
                }
            });
        Root::instance()->FlushRenderCommands();
        mRenderProxy = *Result;
        MarkTransformDirty();
    }

    void StaticMeshComponent::OnUnregisterWorldImpl()
    {
        if (mRenderProxy)
        {
            StaticMeshRenderProxy* ProxyToDestroy = mRenderProxy;
            mRenderProxy = nullptr;

            ENQUEUE_RENDER_COMMAND(DestroyProxy)(
                [ProxyToDestroy, Scene = GetActor()->GetSceneManager()](RenderContext& Context)
                {
                    if (Scene) Scene->DestroyRenderProxy(ProxyToDestroy);
                });
        }
    }

    void StaticMeshComponent::MarkRenderStateDirty()
    {
        if (mRenderProxy == nullptr)
            return;

        // 在逻辑线程生成所有材质的快照
        auto Snapshots = CaptureMaterials();

        StaticMeshRenderProxy* Proxy = mRenderProxy;
        MeshPtr CurrentMesh = mMesh;
        const auto MeshSnapshot = CurrentMesh && CurrentMesh->IsReady() ? CurrentMesh->CreateRenderData() : MeshRenderData{};
        uint32 Count = 0;
        if (!CurrentMesh.isNull())
            Count = CurrentMesh->GetSubMeshCount();

        ENQUEUE_RENDER_COMMAND(UpdateMeshState)(
            [Proxy, CurrentMesh, MeshSnapshot, Snapshots = std::move(Snapshots), Count, Cast = mbCastShadow, Recv = mbReceiveShadow, Layer = mRenderLayer, Visible = mbVisible](RenderContext& Context)
            {
                Proxy->SetMesh(CurrentMesh, MeshSnapshot);
                Proxy->SetSubMeshCount(Count);
                // 用快照数组更新渲染线程的 RenderMaterialProxy
                Proxy->UpdateMaterials(Snapshots, Context);
                Proxy->SetCastShadow(Cast);
                Proxy->SetReceiveShadow(Recv);
                Proxy->SetRenderLayer(Layer);
                Proxy->SetVisible(Visible);
            });
    }

    void StaticMeshComponent::MarkTransformDirty()
    {
        if (mWorld && mWorld->GetSceneBVH()) mWorld->GetSceneBVH()->UpdateObject(GetActor());
        if (mRenderProxy == nullptr)
            return;

        StaticMeshRenderProxy* Proxy = mRenderProxy;
        Matrix4x4 WorldMat = GetActor()->GetWorldMatrix();
        Vector3 WorldPos = GetActor()->GetWorldPosition();
        Vector3 WorldScale = GetActor()->GetWorldScale();
        const auto Bounds = GetBoundingBox();
        auto* Scene = GetActor()->GetSceneManager();

        ENQUEUE_RENDER_COMMAND(UpdateProxy)(
            [=](RenderContext& Context)
            {
                Proxy->SetWorldMatrix(WorldMat);
                Proxy->SetWorldPosition(WorldPos);
                Proxy->SetWorldScale(WorldScale);
                Proxy->SetWorldAABB(Bounds);
                Scene->UpdateRenderProxyBounds(Proxy);
            });
    }

    AxisAlignedBox StaticMeshComponent::GetBoundingBox() const
    {
        AxisAlignedBox Bounds;
        if (mMesh && mMesh->IsReady() && mParent)
        { Bounds = mMesh->GetLocalBounds(); Bounds.transformAffine(mParent->GetWorldMatrix()); }
        return Bounds;
    }

    const Name& StaticMeshComponent::GetType() const
    {
        static Name NameType("StaticMeshComponent");
        return NameType;
    }
}
