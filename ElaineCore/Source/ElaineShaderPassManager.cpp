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
            {"dst_alpha", RHI_BLEND_FACTOR_DST_ALPHA}, {"one_minus_dst_alpha", RHI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA}
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

	void ShaderPassManager::UnloadShaderPass(ShaderPass* InShaderPass, JsonCpp& OutJson)
	{

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

        ShaderPass* NewPass = new ShaderPass(InJson.value("name", "UnnamedPass").c_str());
        NewPass->mRenderQueueName = InJson.value("renderQueue", "opaque");

        // ---------- Shaders ----------
        const JsonCpp& JsonShaders = InJson["shaders"];

        if (JsonShaders.contains("vs"))
        {
            std::string ShaderPath = JsonShaders["vs"];
            ResourceBasePtr ShaderPtr = ShaderManager::instance()->GetResource(ShaderPath, false);
            NewPass->mShaders.push_back({
                EShaderStage::VertexShader,
                ShaderPtr
                });

            //NewPass->AddResourceEvent(ResourceEvent(ShaderPath.c_str(), RT_Shader, ShaderPtr->GetLoadTask()));
        }

        if (JsonShaders.contains("ps"))
        {
            std::string ShaderPath = JsonShaders["ps"];
            ResourceBasePtr ShaderPtr = ShaderManager::instance()->GetResource(ShaderPath, false);
            NewPass->mShaders.push_back({
                EShaderStage::FragmentShader,
                ShaderPtr
                });
            //NewPass->AddResourceEvent(ResourceEvent(ShaderPath.c_str(), RT_Shader, ShaderPtr->GetLoadTask()));
        }

        if (JsonShaders.contains("gs"))
        {
            std::string ShaderPath = JsonShaders["gs"];
            ResourceBasePtr ShaderPtr = ShaderManager::instance()->GetResource(ShaderPath);
            NewPass->mShaders.push_back({
                EShaderStage::GeometryShader,
                ShaderPtr
                });
            //NewPass->AddResourceEvent(ResourceEvent(ShaderPath.c_str(), RT_Shader, ShaderPtr->GetLoadTask()));
        }

        if (JsonShaders.contains("cs"))
        {
            std::string ShaderPath = JsonShaders["cs"];
            ResourceBasePtr ShaderPtr = ShaderManager::instance()->GetResource(ShaderPath);
            NewPass->mShaders.push_back({
                EShaderStage::ComputeShader,
                ShaderPtr
                });
            //NewPass->AddResourceEvent(ResourceEvent(ShaderPath.c_str(), RT_Shader, ShaderPtr->GetLoadTask()));
        }

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
            NewPass->mRHIDesc.mSrcColorBlendFactor = ParseBlendFactor(b.value("srcColor", "src_alpha"));
            NewPass->mRHIDesc.mDstColorBlendFactor = ParseBlendFactor(b.value("dstColor", "one_minus_src_alpha"));
            NewPass->mRHIDesc.mColorBlendOp = ParseBlendOp(b.value("colorOp", "add"));
            NewPass->mRHIDesc.mSrcAlphaBlendFactor = ParseBlendFactor(b.value("srcAlpha", "one"));
            NewPass->mRHIDesc.mDstAlphaBlendFactor = ParseBlendFactor(b.value("dstAlpha", "zero"));
            NewPass->mRHIDesc.mAlphaBlendOp = ParseBlendOp(b.value("alphaOp", "add"));
        }

        // ---------- Hash & PSO ----------
        CalculatePassStateHash(NewPass);
        //CompilePipeline(NewPass);
        NewPass->CompilePipeline();
        mShaderPasses.emplace(NewPass->mRHIDesc.mStateHash, NewPass);
        return NewPass;
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
