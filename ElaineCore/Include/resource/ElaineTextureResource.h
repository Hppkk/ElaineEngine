#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceBase.h"
#include "ElaineTextureBase.h"
#include "render/common/ElaineRHI.h"

namespace Elaine
{
	class RHITexture;

	class ElaineCoreExport Texture :public ResourceBase
	{
	public:
		Texture(ResourceManager* InManager, const std::string& InResName);
		virtual ~Texture() override;
		std::vector<std::string> GetSourceFiles() const override;
        const std::string& GetCubeMapPath(TextureCubeMapFace InFace);
		RHITexture* GetTextureRHI() const { return mTextureRHI.get(); }
	protected:
		void ApplyLoadedData(ResourceLoadData& Data) noexcept override;
        std::unique_ptr<ResourceLoadData> LoadImpl() override;
        bool InitializeGPU(ResourceLoadData& Data) override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;

	private:
        struct LoadData;

		Version mVersion;
		TextureDesc mTextureDesc;
		TextureData mTextureData;
		std::vector<std::string> mTexturePath;
		RHITextureHandle mTextureRHI;
        RHITextureDesc mPendingRHIDesc;
	};

	using TexturePtr = ResourcePtr<Texture>;
}