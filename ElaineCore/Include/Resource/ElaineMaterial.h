#pragma once
#include "ElaineResourceBase.h"
#include "ElainePass.h"
#include "render/common/ElaineRHIProtocol.h"
#include "ElaineTextureResource.h"
#include "ElaineUniformGPUManager.h"
#include "ElaineShader.h"
#include "ElaineVersion.h"
#include "ElaineMaterialInterface.h"

namespace Elaine
{
	class ElaineCoreExport Material : public ResourceBase , public MaterialInterface
	{
	public:
		Material(ResourceManager* InManager, const std::string& InResourceName);
		virtual ~Material();
		void SetTexture(TextureSemantics InSemantics, TexturePtr InTexture);
		TexturePtr GetTexture(TextureSemantics InSemantics) const;
	protected:
		virtual bool LoadImpl() override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;
		virtual void ResourceArrivedImpl() override;
	public:
		ShaderPass* GetPass(const Name& InPassType);
		bool IsRegisterPass(const Name& InPassType);

		// ResourceBase override: 获取所有直接依赖（纹理 + ShaderPass中的Shader）
		void GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const override;

	private:
		Version mVersion;
		TexturePtr mTextures[Tex_Count];
		ShaderPtr mVertexShader;
		ShaderPtr mFragmentShader;
		std::unordered_map<Name, ShaderPass*> mPassMap;
		bool mbDirty = false;
	public:
		
	};

	using MaterialPtr = ResourcePtr<Material>;
}
