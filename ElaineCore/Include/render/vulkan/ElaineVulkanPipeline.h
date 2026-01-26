#pragma once
#include "render/vulkan/ElaineVulkanRenderPassCache.h"

namespace VulkanRHI
{
	class VulkanDescriptorSetLayout;
	class VulkanDevice;
	class VulkanShader;
	class VulkanRenderPass;
	class VulkanDescriptorSet;

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

		const std::vector<VulkanDescriptorSet*>& GetDescriptorSets() const { return mDescriptorSets; }
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayouts() const { return mDescriptorSetLayouts; }
		VulkanDescriptorSet* GetDescriptorSet(int32 InSlot);
	protected:
		VulkanDevice* mDevice = nullptr;
		VkPipeline mPipeline = nullptr;
		VulkanLayout* mLayout = nullptr;
		//VulkanDescriptorSet* mCommonDescriptorSets[MAX_FRAMES_IN_FLIGHT];
		std::vector<VulkanDescriptorSet*> mDescriptorSets;
		std::unordered_map<int32, VulkanDescriptorSet*> mDescriptorSetMap;
		std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
		Type mType = Graphic;
		friend class VulkanShaderManager;
		friend class VulkanShaderCompileManager;
	};

	class ElaineCoreExport VulkanGfxPipeline : public VulkanPipeline
	{
	public:
		VulkanGfxPipeline(VulkanDevice* InDevice);
		virtual ~VulkanGfxPipeline();
		VulkanRenderPass* GetRenderPass() const { return mRenderPass; }
		
		// 获取 Pipeline State (用于创建变体)
		const GRAPHICS_PIPELINE_STATE_DESC& GetPipelineState() const { return mPipelineState; }
		void SetPipelineState(const GRAPHICS_PIPELINE_STATE_DESC& InState) { mPipelineState = InState; }
	private:
		VulkanRenderPass* mRenderPass = nullptr;
		VulkanShader* mVsShader = nullptr;
		VulkanShader* mPsShader = nullptr;
		GRAPHICS_PIPELINE_STATE_DESC mPipelineState;  // 保存 Pipeline State 用于创建变体
		friend class VulkanPiplineManager;
		friend class VulkanShaderCompileManager;
		friend class VulkanShader;
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

	//=============================================================================
	// Pipeline 变体 Key - 用于基于 RenderPass 的 Pipeline 缓存
	//=============================================================================
	struct PipelineVariantKey
	{
		uint64 StateHash = 0;         // GRAPHICS_PIPELINE_STATE_DESC hash
		RenderPassKey RenderPass;      // RenderPass 配置

		bool operator==(const PipelineVariantKey& Other) const
		{
			return StateHash == Other.StateHash && RenderPass == Other.RenderPass;
		}
	};

	struct PipelineVariantKeyHasher
	{
		size_t operator()(const PipelineVariantKey& Key) const
		{
			size_t Hash = std::hash<uint64>()(Key.StateHash);
			RenderPassKeyHasher RPHasher;
			Hash ^= RPHasher(Key.RenderPass) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
			return Hash;
		}
	};

	class ElaineCoreExport VulkanPiplineManager
	{
	public:
		VulkanPiplineManager(VulkanDevice* InDevice);
		~VulkanPiplineManager();
		VulkanComputePipeline* GetOrCreateComputePipeline(VulkanShader* InShader);
		VulkanComputePipeline* CreateComputePipeline(VulkanShader* InShader);
		VulkanGfxPipeline* GetOrCreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InState);
		
		// 新增：根据 RenderPassKey 获取或创建 Pipeline 变体
		VulkanGfxPipeline* GetOrCreateGfxPipelineForRenderPass(
			const GRAPHICS_PIPELINE_STATE_DESC& InState,
			const RenderPassKey& RPKey);
		
		VulkanGfxPipeline* CreateGraphicPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InState);
		
		// 新增：使用指定 RenderPassKey 创建 Pipeline (可复用现有 Pipeline 的 Shader)
		VulkanGfxPipeline* CreateGraphicPipelineWithRenderPass(
			const GRAPHICS_PIPELINE_STATE_DESC& InState,
			const RenderPassKey& RPKey,
			VulkanGfxPipeline* ExistingPipeline = nullptr);
	private:
		VulkanDevice* mDevice = nullptr;
		PipelineCache mPipelineCache;
		std::map<std::string, VulkanComputePipeline*> mComputePipelinePool;
		std::map<uint64, VulkanGfxPipeline*> mGfxPipelinePool;  // 保留向后兼容
		
		// 新增：Pipeline 变体缓存
		std::unordered_map<PipelineVariantKey, VulkanGfxPipeline*, PipelineVariantKeyHasher> mPipelineVariantCache;
	};
}