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
	enum class MaterialParameterKind : uint8_t { Scalar, Vector, Color, Integer, Boolean, Texture, Unknown };
	struct MaterialParameterDesc
	{
		std::string Name;
		uint64_t Hash = 0;
		MaterialParameterKind Kind = MaterialParameterKind::Unknown;
		Vector4 DefaultValue = Vector4::ZERO;
		float Min = 0.0f;
		float Max = 1.0f;
		bool HasRange = false;
		uint32_t Offset = 0;
		uint32_t Size = 0;
		uint32_t ArrayCount = 1;
		uint32_t Set = 2;
		uint32_t Binding = 5;
		bool Reflected = false;
		bool HasDefault = false;
	};

	class ElaineCoreExport Material : public ResourceBase , public MaterialInterface
	{
	public:
		Material(ResourceManager* InManager, const std::string& InResourceName);
		virtual ~Material();
		void SetTexture(TextureSemantics InSemantics, TexturePtr InTexture);
		TexturePtr GetTexture(TextureSemantics InSemantics) const;
		float GetScalar(uint64_t InParamHash, float InDefault = 0.0f) const;
		Vector4 GetVector(uint64_t InParamHash, const Vector4& InDefault = Vector4::ZERO) const;
		int32_t GetInt(uint64_t InParamHash, int32_t InDefault = 0) const;
		bool GetBool(uint64_t InParamHash, bool InDefault = false) const;
		bool HasScalar(uint64_t InParamHash) const;
		bool HasVector(uint64_t InParamHash) const;
		const std::vector<MaterialParameterDesc>& GetParameterLayout() const { return mParameterLayout; }
		std::vector<MaterialParameterDesc> GetParameterLayoutSnapshot() const;
		uint64_t GetLayoutRevision() const { return mLayoutRevision.load(); }
	protected:
		std::unique_ptr<ResourceLoadData> LoadImpl() override;
        bool InitializeGPU(ResourceLoadData& Data) override;
        void PrepareDependencies(ResourceLoadData& Data) override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;
	public:
		ShaderPass* GetPass(const Name& InPassType);
        ShaderPass* GetShaderPass() override;
		bool IsRegisterPass(const Name& InPassType);

		// ResourceBase override: 获取所有直接依赖（纹理 + ShaderPass中的Shader）
		void GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const override;

	private:
        friend struct MaterialParamSnapshot;
        struct LoadData;
        void ApplyLoadedData(ResourceLoadData& Data) noexcept override;
		Version mVersion;
		TexturePtr mTextures[Tex_Count];
		ShaderPtr mVertexShader;
		ShaderPtr mFragmentShader;
		std::unordered_map<Name, ShaderPass*> mPassMap;
		std::unordered_map<uint64_t, float> mScalarDefaults;
		std::unordered_map<uint64_t, Vector4> mVectorDefaults;
		std::unordered_map<uint64_t, int32_t> mIntDefaults;
		std::unordered_map<uint64_t, bool> mBoolDefaults;
		std::vector<MaterialParameterDesc> mParameterLayout;
		bool mbDirty = false;
		std::atomic<uint64_t> mLayoutRevision{0};
		mutable std::mutex mDataMutex;
	public:
		
	};

	using MaterialPtr = ResourcePtr<Material>;
}
