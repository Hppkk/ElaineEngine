#include "ElainePrecompiledHeader.h"
#include "ElainePass.h"
#include "ElaineShaderPassManager.h"
#include "ElaineShaderManager.h"

namespace Elaine
{

    inline static RHICompareOp ParseDepthFunc(const std::string& func)
    {
        static const std::unordered_map<std::string, RHICompareOp> sMap =
        {
            { "never",          COMPARE_OP_NEVER },
            { "less",           COMPARE_OP_LESS },
            { "equal",          COMPARE_OP_EQUAL },
            { "less_equal",     COMPARE_OP_LESS_OR_EQUAL },
            { "lequal",         COMPARE_OP_LESS_OR_EQUAL },
            { "greater",        COMPARE_OP_GREATER },
            { "not_equal",      COMPARE_OP_NOT_EQUAL },
            { "greater_equal",  COMPARE_OP_GREATER_OR_EQUAL },
            { "gequal",         COMPARE_OP_GREATER_OR_EQUAL },
            { "always",         COMPARE_OP_ALWAYS }
        };

        auto it = sMap.find(func);
        if (it != sMap.end())
            return it->second;

        return COMPARE_OP_LESS_OR_EQUAL;
    }

    inline static RHICullMode ParseCullMode(const std::string& value)
    {
        static const std::unordered_map<std::string, RHICullMode> sMap =
        {
            { "none",   CULL_NONE },
            { "off",    CULL_NONE }, 
            { "front",  CULL_FRONT },
            { "back",   CULL_BACK },
            { "all",    CULL_ALL }
        };

        auto it = sMap.find(value);
        if (it != sMap.end())
            return it->second;

        return CULL_BACK;
    }

    inline static RHIPolygonMode ParseFillMode(const std::string& value)
    {
        static const std::unordered_map<std::string, RHIPolygonMode> sMap =
        {
            { "solid",      POLYGON_FILL },
            { "fill",       POLYGON_FILL },
            { "wireframe",  POLYGON_LINE },
            { "line",       POLYGON_LINE },
            { "point",      POLYGON_POINT }
        };

        auto it = sMap.find(value);
        if (it != sMap.end())
            return it->second;

        return POLYGON_FILL;
    }

    inline static RHIFrontFace ParseFrontFace(const std::string& value)
    {
        return value == "cw" || value == "clockwise" ? FRONT_FACE_CLOCKWISE : FRONT_FACE_COUNTER_CLOCKWISE;
    }

    inline static RHIBlendFactor ParseBlendFactor(const std::string& value)
    {
        static const std::unordered_map<std::string, RHIBlendFactor> Map = {
            {"zero", RHI_BLEND_FACTOR_ZERO}, {"one", RHI_BLEND_FACTOR_ONE},
            {"src_color", RHI_BLEND_FACTOR_SRC_COLOR}, {"one_minus_src_color", RHI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR},
            {"dst_color", RHI_BLEND_FACTOR_DST_COLOR}, {"one_minus_dst_color", RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR},
            {"src_alpha", RHI_BLEND_FACTOR_SRC_ALPHA}, {"one_minus_src_alpha", RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA},
            {"dst_alpha", RHI_BLEND_FACTOR_DST_ALPHA}, {"one_minus_dst_alpha", RHI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA},
            {"constant_color", RHI_BLEND_FACTOR_CONSTANT_COLOR}, {"one_minus_constant_color", RHI_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR},
            {"constant_alpha", RHI_BLEND_FACTOR_CONSTANT_ALPHA}, {"one_minus_constant_alpha", RHI_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA},
            {"src_alpha_saturate", RHI_BLEND_FACTOR_SRC_ALPHA_SATURATE}, {"src1_color", RHI_BLEND_FACTOR_SRC1_COLOR},
            {"one_minus_src1_color", RHI_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR}, {"src1_alpha", RHI_BLEND_FACTOR_SRC1_ALPHA}, {"one_minus_src1_alpha", RHI_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA}
        };
        auto It = Map.find(value);
        return It == Map.end() ? RHI_BLEND_FACTOR_ONE : It->second;
    }

    inline static RHIBlendOp ParseBlendOp(const std::string& value)
    {
        if (value == "subtract") return RHI_BLEND_OP_SUBTRACT;
        if (value == "reverse_subtract") return RHI_BLEND_OP_REVERSE_SUBTRACT;
        if (value == "min") return RHI_BLEND_OP_MIN;
        if (value == "max") return RHI_BLEND_OP_MAX;
        return RHI_BLEND_OP_ADD;
    }

	Elaine::ShaderPassManager::ShaderPassManager()
	{

	}

	ShaderPassManager::~ShaderPassManager()
	{

	}

	bool ShaderPassManager::ParseShaderPass(const JsonCpp& InJson)
	{
		if (!InJson.contains("shaders"))
			return false;

		const JsonCpp& shaders = InJson["shaders"];
		if (!shaders.contains("vs") && !shaders.contains("cs"))
			return false;

		return true;
	}

	void ShaderPassManager::UnloadShaderPass(ShaderPass* Pass, JsonCpp& Json)
    {
        if (!Pass) return;
        const auto& D = Pass->mRHIDesc;
        static const char* Compare[] = {"never","less","equal","less_equal","greater","not_equal","greater_equal","always"};
        static const char* Cull[] = {"none","front","back","all"};
        static const char* Fill[] = {"solid","line","point"};
        static const char* Factors[] = {"zero","one","src_color","one_minus_src_color","dst_color","one_minus_dst_color","src_alpha","one_minus_src_alpha","dst_alpha","one_minus_dst_alpha","constant_color","one_minus_constant_color","constant_alpha","one_minus_constant_alpha","src_alpha_saturate","src1_color","one_minus_src1_color","src1_alpha","one_minus_src1_alpha"};
        static const char* Ops[] = {"add","subtract","reverse_subtract","min","max"};
        if (static_cast<size_t>(D.mDepthOp) >= std::size(Compare) || static_cast<size_t>(D.mCullMode) >= std::size(Cull) ||
            static_cast<size_t>(D.mPolygonMode) >= std::size(Fill) || static_cast<size_t>(D.mColorBlendOp) >= std::size(Ops) || static_cast<size_t>(D.mAlphaBlendOp) >= std::size(Ops))
            throw std::runtime_error("Unsupported serialized material render state");
        auto Factor = [&](RHIBlendFactor V) { if (static_cast<size_t>(V) >= std::size(Factors)) throw std::runtime_error("Unsupported serialized blend factor"); return Factors[V]; };
        Json["name"] = Pass->mPassName.ToString(); Json["renderQueue"] = Pass->mRenderQueueName;
        Json["depth"]["test"] = D.mbDepthTestEnable; Json["depth"]["write"] = D.mbDepthWriteEnable; Json["depth"]["func"] = Compare[D.mDepthOp];
        Json["raster"]["cull"] = Cull[D.mCullMode]; Json["raster"]["fill"] = Fill[D.mPolygonMode];
        Json["raster"]["frontFace"] = D.mFrontFace == FRONT_FACE_CLOCKWISE ? "cw" : "ccw";
        Json["multi_sample_count"] = static_cast<unsigned>(D.mMultiSampleCount);
        auto& Blend = Json["blend"]; Blend["enable"] = D.mbEnableColorBlend;
        Blend["srcColor"] = Factor(D.mSrcColorBlendFactor); Blend["dstColor"] = Factor(D.mDstColorBlendFactor); Blend["colorOp"] = Ops[D.mColorBlendOp];
        Blend["srcAlpha"] = Factor(D.mSrcAlphaBlendFactor); Blend["dstAlpha"] = Factor(D.mDstAlphaBlendFactor); Blend["alphaOp"] = Ops[D.mAlphaBlendOp]; Blend["writeMask"] = D.mColorWriteMask;
        Json["shaders"] = JsonCpp::object();
        for (const auto& Entry : Pass->mShaders)
        {
            const char* Key = Entry.mStage == EShaderStage::VertexShader ? "vs" : Entry.mStage == EShaderStage::FragmentShader ? "ps" : Entry.mStage == EShaderStage::GeometryShader ? "gs" : "cs";
            Json["shaders"][Key] = Entry.mShader->GetReference().ToJson();
        }
        Json["macros"] = JsonCpp::object(); for (const auto& Macro : Pass->mMacros) Json["macros"][Macro] = 1;
    }

	void ShaderPassManager::CalculatePassStateHash(ShaderPass* InShaderPass)
	{
        uint64 hash = 0;

        HashCombine(hash, InShaderPass->mRHIDesc.mbDepthTestEnable);
        HashCombine(hash, InShaderPass->mRHIDesc.mbDepthWriteEnable);
        HashCombine(hash, InShaderPass->mRHIDesc.mDepthOp);

        HashCombine(hash, InShaderPass->mRHIDesc.mCullMode);
        HashCombine(hash, InShaderPass->mRHIDesc.mFrontFace);
        HashCombine(hash, InShaderPass->mRHIDesc.mPolygonMode);

        HashCombine(hash, InShaderPass->mRHIDesc.mbEnableColorBlend);
        HashCombine(hash, InShaderPass->mRHIDesc.mSrcColorBlendFactor);
        HashCombine(hash, InShaderPass->mRHIDesc.mDstColorBlendFactor);
        HashCombine(hash, InShaderPass->mRHIDesc.mColorBlendOp);
        HashCombine(hash, InShaderPass->mRHIDesc.mSrcAlphaBlendFactor);
        HashCombine(hash, InShaderPass->mRHIDesc.mDstAlphaBlendFactor);
        HashCombine(hash, InShaderPass->mRHIDesc.mAlphaBlendOp);
        HashCombine(hash, InShaderPass->mRHIDesc.mColorWriteMask);
        HashCombine(hash, (uint64_t)InShaderPass->mRHIDesc.mMultiSampleCount);

        for (auto& s : InShaderPass->mShaders)
        {
            HashCombine(hash, (uint64_t)s.mStage);
            HashCombine(hash, s.mShader->GetHash());
        }

        for (auto& m : InShaderPass->mMacros)
        {
            HashCombine(hash, FNV_1A_HASH_64(m));
        }

        HashCombine(hash, FNV_1A_HASH_64(InShaderPass->mRenderQueueName));
        InShaderPass->mRHIDesc.mStateHash = hash;
	}

	ShaderPass* ShaderPassManager::CreateShaderPass(const JsonCpp& InJson)
	{
        if (!ParseShaderPass(InJson))
            return nullptr;

        auto OwnedPass = std::make_unique<ShaderPass>(InJson.value("name", "UnnamedPass").c_str());
        auto* NewPass = OwnedPass.get();
        NewPass->mRenderQueueName = InJson.value("renderQueue", "opaque");

        // ---------- Shaders ----------
        const JsonCpp& JsonShaders = InJson["shaders"];

        for (const auto& [Key, Stage] : std::initializer_list<std::pair<const char*, EShaderStage>>{
            {"vs", EShaderStage::VertexShader}, {"ps", EShaderStage::FragmentShader},
            {"gs", EShaderStage::GeometryShader}, {"cs", EShaderStage::ComputeShader}})
            if (JsonShaders.contains(Key)) NewPass->mShaders.push_back({Stage, {}, ResourceReference::FromJson(JsonShaders[Key])});

        // ---------- Macros ----------
        if (InJson.contains("macros"))
        {
            for (auto&& m : InJson["macros"].items())
            {
                std::string Macro = m.key();
                bool Enabled = m.value().get<int>() != 0;
                if (Enabled)
                {
                    NewPass->mMacros.push_back(Macro);
                }
            }
        }

        // ---------- Depth ----------
        if (InJson.contains("depth"))
        {
            const JsonCpp& d = InJson["depth"];
            NewPass->mRHIDesc.mbDepthTestEnable = d.value("test", true);
            NewPass->mRHIDesc.mbDepthWriteEnable = d.value("write", true);
            NewPass->mRHIDesc.mDepthOp = ParseDepthFunc(d.value("func", "less_equal"));
        }

        // ---------- Raster ----------
        if (InJson.contains("raster"))
        {
            const JsonCpp& r = InJson["raster"];
            NewPass->mRHIDesc.mCullMode = ParseCullMode(r.value("cull", "back"));
            NewPass->mRHIDesc.mFrontFace = ParseFrontFace(r.value("frontFace", "ccw"));
            NewPass->mRHIDesc.mPolygonMode = ParseFillMode(r.value("fill", "solid"));
        }

        NewPass->mRHIDesc.mMultiSampleCount = (RHIMultiSampleCount)InJson.value("multi_sample_count", 1);

        // ---------- Blend ----------
        if (InJson.contains("blend"))
        {
            const JsonCpp& b = InJson["blend"];
            NewPass->mRHIDesc.mbEnableColorBlend = b.value("enable", false);
            NewPass->mRHIDesc.mColorWriteMask = b.value("writeMask", 15u);
            NewPass->mRHIDesc.mSrcColorBlendFactor = ParseBlendFactor(b.value("srcColor", "src_alpha"));
            NewPass->mRHIDesc.mDstColorBlendFactor = ParseBlendFactor(b.value("dstColor", "one_minus_src_alpha"));
            NewPass->mRHIDesc.mColorBlendOp = ParseBlendOp(b.value("colorOp", "add"));
            NewPass->mRHIDesc.mSrcAlphaBlendFactor = ParseBlendFactor(b.value("srcAlpha", "one"));
            NewPass->mRHIDesc.mDstAlphaBlendFactor = ParseBlendFactor(b.value("dstAlpha", "zero"));
            NewPass->mRHIDesc.mAlphaBlendOp = ParseBlendOp(b.value("alphaOp", "add"));
        }

        // ---------- Hash & PSO ----------
        // Dependency shader data is captured later on GameThread.
        //CompilePipeline(NewPass);
        // GPU compilation is scheduled by the owning material.
        // Passes are owned by the loading material data until delivery.
        return OwnedPass.release();
	}

    bool ShaderPassManager::SaveShaderPass(ShaderPass* InShaderPass)
    {
        return false;
    }

	ShaderPass* ShaderPassManager::GetShaderPass(const Name& InName) const
	{
		return nullptr;
	}

	void ShaderPassManager::CompilePipeline(ShaderPass* InShaderPass)
	{

	}

	void ShaderPassManager::DestroyShaderPass(ShaderPass* InShaderPass)
	{
		if (InShaderPass == nullptr)
			return;
		for (auto It = mShaderPasses.begin(); It != mShaderPasses.end(); )
		{
			if (It->second == InShaderPass)
				It = mShaderPasses.erase(It);
			else
				++It;
		}
		delete InShaderPass;
	}
}
