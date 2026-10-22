#include "ElainePrecompiledHeader.h"
#include "ElaineSkyComponent.h"
#include "ElaineRenderCommandQueue.h"
#include "RenderProxy/ElaineSkyRenderProxy.h"
#include "ElaineActor.h"
#include "ElaineTextureManager.h"
#include "ElaineMaterialInstanceDynamic.h"
#include "ElaineMaterialParamSnapshot.h"

namespace Elaine
{
    SkyComponent::SkyComponent(Actor* InObject)
        : ActorComponent(InObject)
        , mProxy(nullptr)
        , mExposure(1.0f)
        , mMaterial(nullptr)
    {
        // 创建默认的材质实例（逻辑线程）
        mMaterial = new MaterialInstanceDynamic();
        mMaterialSubscription = mMaterial->ObserveParameterChanges([this] { UpdateMaterialSnapshot(); });
        mMaterial->ChangeMaterial("material_instance/SkyBox.mi");

        SetExposure(mExposure);

        // 在逻辑线程捕获独立的材质参数与 RHI 句柄。
        MaterialParamSnapshot Snapshot = mMaterial->CreateSnapshot();

        ENQUEUE_RENDER_COMMAND(CreateSkyRenderProxy)([self = this, Snapshot = std::move(Snapshot)](RenderContext& InContext)
            {
                RenderProxy* NewProxy = self->GetActor()->GetSceneManager()->CreateRenderProxy(EProxyType::Sky);
                SkyRenderProxy* SkyProxy = static_cast<SkyRenderProxy*>(NewProxy);
                if (SkyProxy)
                {
                    // 用快照更新渲染线程的 RenderMaterialProxy（不再传递 MaterialInstanceDynamic*）
                    SkyProxy->UpdateMaterial(Snapshot, InContext);
                    self->mProxy = SkyProxy;

                    // ========== 资源依赖追踪 ==========
                    // 追踪材质资源（会递归追踪 MaterialInstanceStatic -> Material -> Texture/Shader）
                    if (Snapshot.IsValid())
                    {
                        SkyProxy->TrackResource(Snapshot.Source);
                    }
                    // 开始初始化流程（在所有资源加载完成后自动调用 InitializeResourceBinding）
                    SkyProxy->BeginInitialization();
                }
            });
        Root::instance()->FlushRenderCommands();
    }

    SkyComponent::~SkyComponent()
    {
        mMaterialSubscription.Reset();
        SkyRenderProxy* ProxyCopy = mProxy;
        SceneManager* SceneMgr = GetActor()->GetSceneManager();
        ENQUEUE_RENDER_COMMAND(DestroySkyRenderProxy)([ProxyCopy, SceneMgr](RenderContext& InContext)
            {
                if (ProxyCopy)
                {
                    SceneMgr->DestroyRenderProxy(ProxyCopy);
                }
            });
        mProxy = nullptr;

        // 在逻辑线程删除材质（逻辑线程持有，逻辑线程销毁）
        delete mMaterial;
        mMaterial = nullptr;
    }

    void SkyComponent::UpdateMaterialSnapshot()
    {
        if (!mProxy || !mMaterial) return;
        auto Snapshot = mMaterial->CreateSnapshot();
        SkyRenderProxy* Proxy = mProxy;
        ENQUEUE_RENDER_COMMAND(UpdateSkyMaterial)([Proxy, Snapshot = std::move(Snapshot)](RenderContext& Context) {
            Proxy->UpdateMaterial(Snapshot, Context);
        });
    }

    void SkyComponent::SetCubeTexture(const TexturePtr& InCube)
    {
        mCubeTexture = InCube;
        mMaterial->SetTexture(MaterialParameterNames::SkyTexture(), InCube);
    }

    void SkyComponent::SetCubeTexture(const std::string& InPath)
    {
        SetCubeTexture(TextureManager::instance()->RequestResource(InPath).GetResult().Resource);
    }

    void SkyComponent::SetExposure(float InExposure)
    {
        mExposure = InExposure;
        mMaterial->SetScalar(MaterialParameterNames::SkyExposure(), InExposure);
    }

    void SkyComponent::SetMaterial(const std::string& InMaterialPath)
    {
        if (!mMaterial)
        {
            mMaterial = new MaterialInstanceDynamic();
            mMaterialSubscription = mMaterial->ObserveParameterChanges([this] { UpdateMaterialSnapshot(); });
        }
        mMaterial->ChangeMaterial(InMaterialPath);
        if (mCubeTexture) mMaterial->SetTexture(MaterialParameterNames::SkyTexture(), mCubeTexture);
        SetExposure(mExposure);
    }

    const Name& SkyComponent::GetType() const
    {
        static Name NameType("SkyComponent");
        return NameType;
    }
}
