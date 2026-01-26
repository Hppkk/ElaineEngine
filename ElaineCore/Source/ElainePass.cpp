#include "ElainePrecompiledHeader.h"
#include "ElainePass.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	ShaderPass::ShaderPass(const Name& InPassName)
		: mPassName(InPassName)
	{

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
		for (auto&& ShaderStageIns : mShaders)
		{
			if (ShaderStageIns.mStage == EShaderStage::VertexShader)
			{
				mRHIDesc.mVSShaderCode = ShaderStageIns.mShader->GetShaderCode();
			}
			else if (ShaderStageIns.mStage == EShaderStage::FragmentShader)
			{
				mRHIDesc.mPSShaderCode = ShaderStageIns.mShader->GetShaderCode();
			}
		}
		mPipeline = GetDynamicRHI()->GetDefaultCommandContext()->RHICreateGfxPipeline(mRHIDesc);
	}

	//void ShaderPass::AddResourceEvent(const ResourceEvent& InEvent)
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
}