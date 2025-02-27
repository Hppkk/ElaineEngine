#include "ElainePrecompiledHeader.h"
#include "render/common/ElaineRenderModule.h"
#include "render/vulkan/ElaineVulkanRHI.h"

namespace Elaine
{
	RenderModule::RenderModule()
	{

	}

	RenderModule::~RenderModule()
	{

	}

	DynamicRHI* RenderModule::LoadDynamicRHI(const RHI_PARAM_DESC& InDesc)
	{
		if (mbIsLoaded)
			return mDynamicRHI;

		mbIsLoaded = true;

		switch (InDesc.RHIType)
		{
		case Elaine::Vulkan:
			mDynamicRHI = new VulkanRHI::VulkanDynamicRHI();
			mDynamicRHI->Initilize(InDesc);
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
		return mDynamicRHI;
	}
}