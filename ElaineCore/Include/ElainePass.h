#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineShader.h"
#include "Resource/ElaineResourceDependencyProvider.h"

namespace Elaine
{

	enum PassType : unsigned int
	{
		CustomPass0,
		CustomPass1,
		CustomPass2,
		CustomPass3,
		CustomPass4,
		NormalPass,
		ShadowPass,
		TransparentNormalPass,
		TransparentDepthPass,
		TransparentRenderPass,

		TransparentFrontPass,
		TransparentBackPass,
		PassCount,
	};



	class ElaineCoreExport ShaderPass : public IResourceDependencyProvider
	{
	public:
		ShaderPass(const Name& InPassName);
		const std::string& GetVsMacros() const { return mVsMacros; }
		const std::string& GetPsMacros() const { return mPsMacros; }
		void AppendVsMacros(const std::string& InMacrosString);
		void AppendPsMacros(const std::string& InMacrosString);
		void CompilePipeline();
		GRAPHICS_PIPELINE_STATE_DESC& GetGfxState() { return mRHIDesc; }
		RHIPipeline* GetPipelineRHI() const { return mPipeline; }
		void MarkDirty() { mDirty = true; }
		bool PipelineDirty() const { return mDirty; }
		const Name& GetPassName() const { return mPassName; }
		//const std::vector<ResourceEvent>& GetResourceEvents() const { return mResourceEvents; }
		//void AddResourceEvent(const ResourceEvent& InEvent);

		// IResourceDependencyProvider implementation
		void GetDependentResources(std::vector<ResourceBasePtr>& OutResources) const override;

	private:
		std::string		mVsMacros;
		std::string		mPsMacros;
		Name			mPassName;
		RHIPipeline*	mPipeline = nullptr;
		bool			mDirty = false;
		std::vector<ShaderStageEntry> mShaders;
		std::vector<std::string> mMacros;
		//std::vector<ResourceEvent> mResourceEvents;
	public:
		GRAPHICS_PIPELINE_STATE_DESC mRHIDesc;
		friend class ShaderPassManager;
	};
}
