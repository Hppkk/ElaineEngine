#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceBase.h"
#include "ElaineTextureBase.h"

namespace Elaine
{
	class RHITexture;

	class ElaineCoreExport Texture :public ResourceBase
	{
	public:
		Texture(ResourceManager* InManager, const std::string& InResName);
		virtual ~Texture() override;
		const std::string& GetCubeMapPath(TextureCubeMapFace InFace);
		RHITexture* GetTextureRHI() { return mTextureRHI; }
	protected:
		virtual bool LoadImpl() override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;
		virtual void ResourceArrivedImpl() override;

	private:
		Version mVersion;
		TextureDesc mTextureDesc;
		TextureData mTextureData;
		std::vector<std::string> mTexturePath;
		RHITexture* mTextureRHI = nullptr;
	};

	using TexturePtr = ResourcePtr<Texture>;
}