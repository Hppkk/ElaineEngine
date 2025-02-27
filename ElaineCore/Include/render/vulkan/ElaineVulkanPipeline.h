#pragma once

namespace VulkanRHI
{
	class VulkanDescriptorSetLayout;
	class VulkanDevice;
	class VulkanShader;
	class VulkanRenderPass;

	class ElaineCoreExport PipelineCache
	{
	public:
		VkPipelineCache GetHandle() const { return mHandle; }
	private:
		VkPipelineCache mHandle = nullptr;
	};

	class ElaineCoreExport VulkanLayout
	{
	public:
		VulkanLayout(VulkanDevice* InDevice);
		~VulkanLayout();

		VkPipelineLayout GetPipelineLayoutHandle() const
		{
			return mPipelineLayout;
		}
	private:
		VulkanDevice* mDevice = nullptr;
		VkPipelineLayout mPipelineLayout = nullptr;
		VulkanDescriptorSetLayout* mDescriptorSetLayout = nullptr;
		friend class VulkanPiplineManager;
	};

	class ElaineCoreExport VulkanPipeline : public RHIPipeline
	{
	public:
		enum Type
		{
			Graphic,
			Compute,
		};
	public:
		VulkanPipeline(VulkanDevice* InDevice);
		virtual ~VulkanPipeline();

		inline VkPipeline GetHandle() const
		{
			return mPipeline;
		}

		Type GetType() const
		{
			return mType;
		}

		inline const VulkanLayout& GetLayout() const
		{
			return *mLayout;
		}
	protected:
		VulkanDevice* mDevice = nullptr;
		VkPipeline mPipeline = nullptr;
		VulkanLayout* mLayout = nullptr;
		Type mType = Graphic;
	};

	class ElaineCoreExport VulkanGfxPipeline : public VulkanPipeline
	{
	public:
		VulkanGfxPipeline(VulkanDevice* InDevice);
		virtual ~VulkanGfxPipeline();
		VulkanRenderPass* GetRenderPass() const { return mRenderPass; }
	private:
		VulkanRenderPass* mRenderPass = nullptr;
		VulkanShader* mVsShader = nullptr;
		VulkanShader* mPsShader = nullptr;
		friend class VulkanPiplineManager;
	};

	class ElaineCoreExport VulkanComputePipeline : public VulkanPipeline
	{
	public:
		VulkanComputePipeline(VulkanDevice* InDevice);
		virtual ~VulkanComputePipeline();
	private:
		VulkanShader* mComputeShader = nullptr;
		friend class VulkanPiplineManager;
	};

	class ElaineCoreExport VulkanPiplineManager
	{
	public:
		VulkanPiplineManager(VulkanDevice* InDevice);
		~VulkanPiplineManager();
		VulkanComputePipeline* GetOrCreateComputePipeline(VulkanShader* InShader);
		VulkanComputePipeline* CreateComputePipeline(VulkanShader* InShader);
		VulkanGfxPipeline* GetOrCreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InState);
		VulkanGfxPipeline* CreateGraphicPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InState);
	private:
		VulkanDevice* mDevice = nullptr;
		PipelineCache mPipelineCache;
		std::map<std::string, VulkanComputePipeline*> mComputePipelinePool;
		std::map<uint64, VulkanGfxPipeline*> mGfxPipelinePool;
	};
}