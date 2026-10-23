#include "ElainePrecompiledHeader.h"
#include "ElainePass.h"
#include "ElaineShaderPassManager.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHI.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	ShaderPass::ShaderPass(const Name& InPassName)
		: mPassName(InPassName)
	{

	}

    ShaderPass::~ShaderPass() = default;
    void ShaderPass::PrepareDependencies(const ResourceLoadData& Data)
    {
		for (auto&& ShaderStageIns : mShaders)
		{
            ShaderStageIns.mShader = static_pointer_cast<Shader>(Data.GetDependency(ShaderStageIns.mReference, "Shader"));
            if (ShaderStageIns.mShader->GetStage() != ShaderStageIns.mStage) throw std::runtime_error("Shader stage does not match material pass");
			if (ShaderStageIns.mStage == EShaderStage::VertexShader)
			{
				mRHIDesc.mVSShaderCode = ShaderStageIns.mShader->GetShaderCode();
                mRHIDesc.mVSPrepared = ShaderStageIns.mShader->GetPreparedShader();
			}
			else if (ShaderStageIns.mStage == EShaderStage::FragmentShader)
			{
				mRHIDesc.mPSShaderCode = ShaderStageIns.mShader->GetShaderCode();
                mRHIDesc.mPSPrepared = ShaderStageIns.mShader->GetPreparedShader();
			}
		}
        ShaderPassManager::instance()->CalculatePassStateHash(this);
    }

	void ShaderPass::AppendVsMacros(const std::string& InMacrosString)
	{
		if (InMacrosString.empty())
			return;

		mVsMacros.append(InMacrosString);
	}

	void ShaderPass::AppendPsMacros(const std::string& InMacrosString)
	{
		if (InMacrosString.empty())
			return;

		mPsMacros.append(InMacrosString);
	}

	void ShaderPass::CompilePipeline()
    {
        CheckRHICreationThread();
        mPipeline = GetDynamicRHI()->GetDefaultCommandContext()->RHICreateGfxPipeline(mRHIDesc);
        if (!mPipeline) throw std::runtime_error("Shader pipeline compilation failed");
    }

	//{
	//	mResourceEvents.push_back(InEvent);
	//}

	void ShaderPass::GetDependentResources(std::vector<ResourceBasePtr>& OutResources) const
	{
		for (const auto& Entry : mShaders)
		{
			if (!Entry.mShader.isNull())
			{
				OutResources.push_back(Entry.mShader);
			}
		}
	}
    ShaderPass ShaderPass::CreateSnapshot() const
    {
        ShaderPass Copy(*this);
        Copy.mShaders.clear();
        return Copy;
    }

}