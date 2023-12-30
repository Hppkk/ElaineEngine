#include "ElainePrecompiledHeader.h"
#include "render/ElaineRenderSystem.h"

namespace Elaine
{
	RenderSystem::RenderSystem()
	{

	}

	void RenderSystem::initilize(RHITYPE type)
	{
		switch (type)
		{
		case Elaine::Vulkan:
		{
			m_pRHI = new VulkanRHI();
			RHIInitInfo info;
			if (Root::instance()->GetEngineMode() == em_Editor)
				info.windowname = "ElaineEditor";
			else
				info.windowname = "Runtime";
			m_pRHI->initialize(&info);
		}
			break;
		case Elaine::Dx11:
			break;
		case Elaine::Dx12:
			break;
		case Elaine::Metal:
			break;
		case Elaine::OpenGl:
			break;
		default:
			break;
		}


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