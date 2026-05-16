#include "ElainePrecompiledHeader.h"
#include "ElaineSkyComponent.h"
#include "ElaineRenderCommandQueue.h"
#include "RenderProxy/ElaineSkyRenderProxy.h"
#include "ElaineGameObject.h"
#include "ElaineTextureManager.h"
#include "ElaineMaterialInstanceDynamic.h"
#include "ElaineMaterialParamSnapshot.h"

namespace Elaine
{
    SkyComponent::SkyComponent(GameObject* InObject)
        : Component(InObject)
        , mProxy(nullptr)
        , mExposure(1.0f)
        , mMaterial(nullptr)
    {
        // 创建默认的材质实例（逻辑线程）
        mMaterial = new MaterialInstanceDynamic();
        mMaterial->ChangeMaterial("material_instance/SkyBox.mi");

        // 在逻辑线程生成材质参数快照（不含 RHI 资源）
        MaterialParamSnapshot Snapshot = mMaterial->CreateSnapshot();
        TexturePtr CubeCopy = mCubeTexture;
        float ExposureCopy = mExposure;

        ENQUEUE_RENDER_COMMAND(CreateSkyRenderProxy)([self = this, Snapshot = std::move(Snapshot), CubeCopy, ExposureCopy](RenderContext& InContext)
            {
                RenderProxy* NewProxy = self->GetGameObject()->GetSceneManager()->CreateRenderProxy(EProxyType::Sky);
                SkyRenderProxy* SkyProxy = static_cast<SkyRenderProxy*>(NewProxy);
                if (SkyProxy)
                {
                    SkyProxy->SetExposure(ExposureCopy);
                    if (!CubeCopy.isNull())
                        SkyProxy->SetCubeTexture(CubeCopy);

                    // 用快照更新渲染线程的 RenderMaterialProxy（不再传递 MaterialInstanceDynamic*）
                    SkyProxy->UpdateMaterial(Snapshot);
                    self->mProxy = SkyProxy;

                    // ========== 资源依赖追踪 ==========
                    // 追踪材质资源（会递归追踪 MaterialInstanceStatic -> Material -> Texture/Shader）
                    if (Snapshot.IsValid())
                    {
                        SkyProxy->TrackResource(Snapshot.Source);
                    }
                    // 追踪 CubeTexture
                    if (!CubeCopy.isNull())
                    {
                        SkyProxy->TrackResource(CubeCopy);
                    }
                    // 开始初始化流程（在所有资源加载完成后自动调用 InitializeResourceBinding）
                    SkyProxy->BeginInitialization();
                }
            });
    }

    SkyComponent::~SkyComponent()
    {
        SkyRenderProxy* ProxyCopy = mProxy;
        SceneManager* SceneMgr = GetGameObject()->GetSceneManager();
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

    void SkyComponent::SetCubeTexture(const TexturePtr& InCube)
    {
        mCubeTexture = InCube;
        SkyRenderProxy* ProxyCopy = mProxy;
        TexturePtr CubeCopy = mCubeTexture;
        ENQUEUE_RENDER_COMMAND(UpdateSkyCube)([ProxyCopy, CubeCopy](RenderContext& InContext)
            {
                if (ProxyCopy)
                    ProxyCopy->SetCubeTexture(CubeCopy);
            });
    }

    void SkyComponent::SetCubeTexture(const std::string& InPath)
    {
        mCubeTexture = TextureManager::instance()->GetResource(InPath);
        SkyRenderProxy* ProxyCopy = mProxy;
        TexturePtr CubeCopy = mCubeTexture;
        ENQUEUE_RENDER_COMMAND(UpdateSkyCube)([ProxyCopy, CubeCopy](RenderContext& InContext)
            {
                if (ProxyCopy)
                    ProxyCopy->SetCubeTexture(CubeCopy);
            });
    }

    void SkyComponent::SetExposure(float InExposure)
    {
        mExposure = InExposure;
        SkyRenderProxy* ProxyCopy = mProxy;
        ENQUEUE_RENDER_COMMAND(UpdateSkyExposure)([ProxyCopy, InExposure](RenderContext& InContext)
            {
                if (ProxyCopy)
                    ProxyCopy->SetExposure(InExposure);
            });
    }

    void SkyComponent::SetMaterial(const std::string& InMaterialPath)
    {
        if (mMaterial)
        {
            mMaterial->ChangeMaterial(InMaterialPath);
        }
        else
        {
            mMaterial = new MaterialInstanceDynamic();
            mMaterial->ChangeMaterial(InMaterialPath);
        }

        // 在逻辑线程生成快照，传递给渲染线程
        MaterialParamSnapshot Snapshot = mMaterial->CreateSnapshot();
        SkyRenderProxy* ProxyCopy = mProxy;
        ENQUEUE_RENDER_COMMAND(UpdateSkyMaterial)([ProxyCopy, Snapshot = std::move(Snapshot)](RenderContext& InContext)
            {
                if (ProxyCopy)
                    ProxyCopy->UpdateMaterial(Snapshot);
            });
    }

    const Name& SkyComponent::GetType() const
    {
        static Name NameType("SkyComponent");
        return NameType;
    }
}