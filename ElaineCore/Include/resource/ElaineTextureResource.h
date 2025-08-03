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
		Texture(ResourceManager* InManager, TextureDimension InDimension, 
			const std::string& InResName, const std::vector<std::string> InCubePaths = {});
		virtual ~Texture() override;
		virtual void			loadImpl() override;
		virtual	void			unloadImpl() override;

		const std::string& GetCubeMapPath(TextureCubeMapFace InFace);
		RHITexture* GetTextureRHI() { return mTextureRHI; }
	private:
		TextureData mTextureData;
		TextureDimension mDimension = TextureDimension::Texture2D;
		std::string mCubePaths[TEXTURE_CUBE_MAP_MAX];
		RHITexture* mTextureRHI = nullptr;
	};
}