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
        const TextureDesc& GetTextureDesc() const { return mTextureDesc; }
		RHITexture* GetTextureRHI() const { return mTextureRHI.get(); }
	protected:
		void ApplyLoadedData(ResourceLoadData& Data) noexcept override;
        std::unique_ptr<ResourceLoadData> LoadImpl() override;
        bool InitializeGPU(ResourceLoadData& Data) override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;

	private:
        struct LoadData;

		TextureDesc mTextureDesc;
		TextureData mTextureData;
		RHITextureHandle mTextureRHI;
        RHITextureDesc mPendingRHIDesc;
	};

	using TexturePtr = ResourcePtr<Texture>;
}