#pragma once
#include "GamePlay/ElaineComponent.h"
#include "ElaineTextureResource.h"
#include "GamePlay/ElaineComponentFactory.h"

namespace Elaine
{
    class SceneManager;
    class SkyRenderProxy;
    class MaterialInstanceDynamic;

    class ElaineEngineExport SkyComponentInfo : public ComponentInfo
    {
    public:
    };

    class ElaineEngineExport SkyComponent : public Component
    {
    public:
        SkyComponent(GameObject* InObject);
        ~SkyComponent();

        void SetCubeTexture(const TexturePtr& InCube);
        void SetCubeTexture(const std::string& InPath);
        void SetExposure(float InExposure);
        void SetMaterial(const std::string& InMaterialPath);
        const Name& GetType() const override;
    private:
        SkyRenderProxy* mProxy = nullptr;
        TexturePtr mCubeTexture;
        float mExposure = 1.0f;
        MaterialInstanceDynamic* mMaterial = nullptr;
    };

    DEFINE_COM_FACTORY(SkyComponent);

}
