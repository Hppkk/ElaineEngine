#include "ElainePrecompiledHeader.h"
#include "render/ElaineRenderSystem.h"

namespace Elaine
{
	RenderSystem::RenderSystem()
	{

	}

	void RenderSystem::initilize()
	{
		m_pRHI = new VulkanRHI();
		RHIInitInfo info;
		if (Root::instance()->GetEngineMode() == em_Editor)
			info.windowname = "ElaineEditor";
		else
			info.windowname = "Runtime";
		m_pRHI->initialize(&info);

	}

	RenderSystem::~RenderSystem()
	{
		SAFE_DELETE(m_pRHI)
	}

	void RenderSystem::tick(float dt)
	{

	}
	void RenderSystem::clear()
	{

	}
	void RenderSystem::swapLogicRenderData()
	{

	}

}