#include "ElainePrecompiledHeader.h"
#include "ElaineSkyComponent.h"
#include "ElaineRenderCommandQueue.h"
#include "RenderProxy/ElaineSkyRenderProxy.h"
#include "ElaineGameObject.h"
#include "ElaineTextureManager.h"
#include "ElaineMaterialInstanceDynamic.h"

namespace Elaine
{
    SkyComponent::SkyComponent(GameObject* InObject)
        : Component(InObject)
        , mProxy(nullptr)
        , mExposure(1.0f)
        , mMaterial(nullptr)
    {
        // 创建默认的材质实例
        mMaterial = new MaterialInstanceDynamic();
        mMaterial->ChangeMaterial("material_instance/SkyBox.mi");

        ENQUEUE_RENDER_COMMAND(CreateSkyRenderProxy)([self = this](RenderContext& InContext)
            {

                RenderProxy* NewProxy = self->GetGameObject()->GetSceneManager()->CreateRenderProxy(EProxyType::Sky);
                SkyRenderProxy* SkyProxy = static_cast<SkyRenderProxy*>(NewProxy);
                if (SkyProxy)
                {
                    SkyProxy->SetExposure(self->mExposure);
                    if (!self->mCubeTexture.isNull())
                        SkyProxy->SetCubeTexture(self->mCubeTexture);
                    SkyProxy->SetMaterial(self->mMaterial);
                    self->mProxy = SkyProxy;

                    // ========== 资源依赖追踪 ==========
                    // 追踪材质资源（会递归追踪 MaterialInstanceStatic -> Material -> Texture/Shader）
                    if (self->mMaterial && !self->mMaterial->GetSourceMaterial().isNull())
                    {
                        SkyProxy->TrackResource(self->mMaterial->GetSourceMaterial());
                    }
                    // 追踪 CubeTexture
                    if (!self->mCubeTexture.isNull())
                    {
                        SkyProxy->TrackResource(self->mCubeTexture);
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
        MaterialInstanceDynamic* MaterialCopy = mMaterial;
        ENQUEUE_RENDER_COMMAND(DestroySkyRenderProxy)([ProxyCopy, SceneMgr, MaterialCopy](RenderContext& InContext)
            {
                if (ProxyCopy)
                {
                    SceneMgr->DestroyRenderProxy(ProxyCopy);
                }
                // 注意: mMaterial的生命周期由逻辑线程管理，这里不删除
                // 在渲染线程执行后，ProxyCopy已被销毁，不会再访问MaterialCopy
            });
        mProxy = nullptr;

        // 在逻辑线程删除材质
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

        SkyRenderProxy* ProxyCopy = mProxy;
        MaterialInstanceDynamic* MaterialCopy = mMaterial;
        ENQUEUE_RENDER_COMMAND(UpdateSkyMaterial)([ProxyCopy, MaterialCopy](RenderContext& InContext)
            {
                if (ProxyCopy)
                    ProxyCopy->SetMaterial(MaterialCopy);
            });
    }

    const Name& SkyComponent::GetType() const
    {
        static Name NameType("SkyComponent");
        return NameType;
    }
}
