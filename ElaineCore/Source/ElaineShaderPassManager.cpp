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
        HashCombine(hash, InShaderPass->mRHIDesc.mPolygonMode);

        HashCombine(hash, InShaderPass->mRHIDesc.mbEnableColorBlend);
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

        InShaderPass->mRHIDesc.mStateHash = hash;
	}

	ShaderPass* ShaderPassManager::CreateShaderPass(const JsonCpp& InJson)
	{
        if (!ParseShaderPass(InJson))
            return nullptr;

        ShaderPass* NewPass = new ShaderPass(InJson.value("name", "UnnamedPass").c_str());

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
        if (InJson.contains("depthstate"))
        {
            const JsonCpp& d = InJson["depthstate"];
            NewPass->mRHIDesc.mbDepthTestEnable = d.value("test", true);
            NewPass->mRHIDesc.mbDepthWriteEnable = d.value("write", true);
            NewPass->mRHIDesc.mDepthOp = ParseDepthFunc(d.value("func", "less_equal"));
        }

        // ---------- Raster ----------
        if (InJson.contains("raster"))
        {
            const JsonCpp& r = InJson["raster"];
            NewPass->mRHIDesc.mCullMode = ParseCullMode(r.value("cull_mode", "back"));
            NewPass->mRHIDesc.mPolygonMode = ParseFillMode(r.value("polygon_mode", "solid"));
        }

        NewPass->mRHIDesc.mMultiSampleCount = (RHIMultiSampleCount)InJson.value("multi_sample_count", 1);

        // ---------- Blend ----------
        if (InJson.contains("blend"))
        {
            const JsonCpp& b = InJson["blend"];
            NewPass->mRHIDesc.mbEnableColorBlend = b.value("enable", false);
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

	}
}