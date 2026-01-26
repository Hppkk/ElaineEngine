#pragma once
#include "render/vulkan/ElaineVulkanTypes.h"
#include "ElaineCorePrerequirements.h"

namespace VulkanRHI
{
	class VulkanDevice;
	class ElaineCoreExport VulkanViewport :public Elaine::RHIViewport
	{
	public:
		VulkanViewport(VulkanDevice* InDevice, uint32 InSizeX, uint32 InSizeY);
		virtual ~VulkanViewport();
		void GetViewSize(uint32& InOutSizeX, uint32& InOutSizeY) const { InOutSizeX = mSizeX; InOutSizeY = mSizeY; }
		void Resize(uint32 InSizeX, uint32 InSizeY);
		const VkViewport& GetDefaultViewPort() const { return mVkViewPort; }
		const VkRect2D& GetDefaultScissor() const { return mScissor; }
		void SetSize(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
	private:
		uint32 mSizeX = 0;
		uint32 mSizeY = 0;
		VulkanDevice* mDevice = nullptr;
		VkViewport mVkViewPort;
		VkRect2D mScissor;
	};
}