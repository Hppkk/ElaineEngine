#pragma once
#include "GamePlay/ElaineComponent.h"
#include "ElaineTextureResource.h"
#include "GamePlay/ElaineComponentFactory.h"
#include "ElaineReflectionDefines.h"
#include "ElaineSkyComponent.generated.h"

namespace Elaine
{
    class SceneManager;
    class SkyRenderProxy;
    class MaterialInstanceDynamic;

    class ElaineEngineExport SkyComponentInfo : public ComponentInfo
    {
    public:
    };

    ECLASS(DisplayName = "Sky")
    class ElaineEngineExport SkyComponent : public Component
    {
        GENERATED_BODY()
    public:
        SkyComponent(GameObject* InObject);
        ~SkyComponent();

        EFUNCTION(Category="Sky")
        void SetCubeTexture(const TexturePtr& InCube);
        EFUNCTION(Category="Sky")
        void SetCubeTexture(const std::string& InPath);
        EFUNCTION(Category="Sky")
        void SetExposure(float InExposure);
        EFUNCTION(Category="Sky")
        void SetMaterial(const std::string& InMaterialPath);
        const Name& GetType() const override;
    private:
        SkyRenderProxy* mProxy = nullptr;
        TexturePtr mCubeTexture;
        EPROPERTY(DisplayName="Exposure", Category="Sky", Tooltip="Sky exposure multiplier", Min=0.0, Max=20.0)
        float mExposure = 1.0f;
        MaterialInstanceDynamic* mMaterial = nullptr;
    };

    DEFINE_COM_FACTORY(SkyComponent);

}
