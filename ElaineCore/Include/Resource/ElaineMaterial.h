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
	enum class MaterialParameterKind : uint8_t { Scalar, Vector, Color, Integer, Boolean, Texture, Buffer, Matrix, Unknown };
	struct MaterialParameterDesc
	{
		std::string Name;
		uint64_t Hash = 0;
		MaterialParameterKind Kind = MaterialParameterKind::Unknown;
		Vector4 DefaultValue = Vector4::ZERO;
        Matrix4x4 DefaultMatrix;
		float Min = 0.0f;
		float Max = 1.0f;
		bool HasRange = false;
        RHIDefaultTexture DefaultTexture = RHIDefaultTexture::None;
		bool HasDefault = false;
        std::string TexturePresence; // Internal derived value, evaluated before texture fallbacks.
	};

	class ElaineCoreExport Material : public ResourceBase , public MaterialInterface
	{
	public:
		Material(ResourceManager* InManager, const std::string& InResourceName);
		virtual ~Material();
		void SetTexture(TextureSemantics InSemantics, TexturePtr InTexture) { SetTexture(SemanticsRegister::GetName(InSemantics), InTexture); }
        void SetTexture(const Name& InName, TexturePtr InTexture);
        TexturePtr GetTexture(const Name& InName) const;
        void SetReadOnlyBuffer(const Name& InName, const RHIBufferView& View);
		TexturePtr GetTexture(TextureSemantics InSemantics) const { return GetTexture(SemanticsRegister::GetName(InSemantics)); }
		float GetScalar(uint64_t InParamHash, float InDefault = 0.0f) const;
		Vector4 GetVector(uint64_t InParamHash, const Vector4& InDefault = Vector4::ZERO) const;
        Matrix4x4 GetMatrix(uint64_t InParamHash, const Matrix4x4& InDefault = Matrix4x4::IDENTITY) const;
		int32_t GetInt(uint64_t InParamHash, int32_t InDefault = 0) const;
		bool GetBool(uint64_t InParamHash, bool InDefault = false) const;
		bool HasScalar(uint64_t InParamHash) const;
		bool HasVector(uint64_t InParamHash) const;
		const std::vector<MaterialParameterDesc>& GetParameterLayout() const { return mPublicParameterLayout; }
		std::vector<MaterialParameterDesc> GetParameterLayoutSnapshot() const;
		uint64_t GetLayoutRevision() const { return mLayoutRevision.load(); }
        uint64_t GetContentRevision() const { return mContentRevision; }
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
        JsonCpp mSavedJson;
        void ApplyLoadedData(ResourceLoadData& Data) noexcept override;
		Version mVersion;
		std::unordered_map<std::string, TexturePtr> mTextures;
        std::unordered_map<std::string, RHIBufferView> mReadOnlyBuffers;
		ShaderPtr mVertexShader;
		ShaderPtr mFragmentShader;
		std::unordered_map<Name, ShaderPass*> mPassMap;
		std::unordered_map<uint64_t, float> mScalarDefaults;
		std::unordered_map<uint64_t, Vector4> mVectorDefaults;
        std::unordered_map<uint64_t, Matrix4x4> mMatrixDefaults;
		std::unordered_map<uint64_t, int32_t> mIntDefaults;
		std::unordered_map<uint64_t, bool> mBoolDefaults;
		std::vector<MaterialParameterDesc> mParameterLayout, mPublicParameterLayout;
		bool mbDirty = false;
		std::atomic<uint64_t> mLayoutRevision{0};
        uint64_t mContentRevision = 0;
		mutable std::mutex mDataMutex;
	public:
		
	};

	using MaterialPtr = ResourcePtr<Material>;
}
