#include "ElainePrecompiledHeader.h"
#include "render/common/ElaineRenderModule.h"
#include "render/vulkan/ElaineVulkanRHI.h"
#include "ElaineRenderSystem.h"
#include "ElaineUniformGPUManager.h"
#include "ElaineShaderPassManager.h"
#include "ElaineRenderGraph.h"

namespace Elaine
{
	RenderModule::RenderModule()
	{

	}

	RenderModule::~RenderModule()
	{

	}

	void RenderModule::LoadDynamicRHI(const RHI_PARAM_DESC& InDesc)
	{
		if (mbIsLoaded)
			return;
		mbIsLoaded = true;
		RenderSystem::instance()->Initialize(InDesc);
	}

	void RenderModule::Initialize()
	{
		SemanticsRegister::Initialize();
		new RenderSystem();
		new ShaderPassManager();
		new RenderGraph::RenderDependencyGraph();
	}

	void RenderModule::Terminate()
	{

	}
}