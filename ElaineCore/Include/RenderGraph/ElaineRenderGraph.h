#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include "RenderGraph/ElaineRenderGraphResource.h"
#include "RenderGraph/ElaineRenderPass.h"
#include "RenderGraph/ElaineRenderGraphBuilder.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>

namespace Elaine
{
	class RHICommandContext;
	class RHITexture;
	class RHIBuffer;
}

namespace RenderGraph
{
	class RenderGraphResourcePool;

	//=============================================================================
	// 资源屏障信息
	//=============================================================================
	struct RGBarrier
	{
		RGResourceHandle Resource;
		Elaine::ERHIAccess StateBefore;
		Elaine::ERHIAccess StateAfter;
		ERGQueueType QueueBefore = ERGQueueType::Graphics;
		ERGQueueType QueueAfter = ERGQueueType::Graphics;
	};

	//=============================================================================
	// 编译后的 Pass 信息
	//=============================================================================
	struct CompiledPass
	{
		RGPass* Pass = nullptr;
		std::vector<RGBarrier> BarriersBefore;
		std::vector<RGBarrier> BarriersAfter;
		ERGQueueType Queue = ERGQueueType::Graphics;
		bool IsCulled = false;
	};

	//=============================================================================
	// 异步计算同步点
	//=============================================================================
	struct AsyncComputeSync
	{
		uint32 GraphicsPassIndex = 0;	// 图形队列需要等待的 Pass
		uint32 ComputePassIndex = 0;	// 计算队列需要等待的 Pass
		bool GraphicsWaitsCompute = false;
		bool ComputeWaitsGraphics = false;
	};

	//=============================================================================
	// RenderDependencyGraph - 核心 RenderGraph 实现
	//=============================================================================
	class ElaineCoreExport RenderDependencyGraph : public Elaine::Singleton<RenderDependencyGraph>
	{
	public:
		RenderDependencyGraph();
		~RenderDependencyGraph();

		// 开始新的一帧
		void BeginFrame();

		// 编译图
		void Compile();

		// 执行图
		void Execute(Elaine::RHICommandContext* GraphicsContext,
			Elaine::RHICommandContext* ComputeContext = nullptr);

		// 结束当前帧
		void EndFrame();

		// 添加 Pass
		void AddPass(std::unique_ptr<RGPass> Pass);

		// 根据索引获取 Pass
		RGPass* GetPass(uint32 Index) const;

		// 获取 Pass 数量
		uint32 GetPassCount() const { return static_cast<uint32>(mPasses.size()); }

		// 创建瞬态纹理
		RGTextureHandle CreateTexture(const std::string& Name, const RGTextureDesc& Desc);

		// 创建瞬态缓冲区
		RGBufferHandle CreateBuffer(const std::string& Name, const RGBufferDesc& Desc);

		// 创建持久纹理（常驻内存，不参与帧末重置）
		RGTextureHandle CreatePersistentTexture(const std::string& Name, const RGTextureDesc& Desc);

		// 创建持久缓冲区
		RGBufferHandle CreatePersistentBuffer(const std::string& Name, const RGBufferDesc& Desc);

		// 导入外部纹理
		RGTextureHandle ImportTexture(const std::string& Name, Elaine::RHITexture* Texture,
			const RGTextureDesc& Desc);

		// 导入外部缓冲区
		RGBufferHandle ImportBuffer(const std::string& Name, Elaine::RHIBuffer* Buffer,
			const RGBufferDesc& Desc);

		// 创建纹理别名
		RGTextureHandle CreateTextureAlias(const std::string& Name, const RGResourceAlias& Alias);

		// 更新资源生命周期
		void UpdateResourceLifetime(RGResourceHandle Handle, uint32 PassIndex, ERGResourceAccess Access);

		//=========================================================================
		// Extracted 资源 - 跨帧传递
		//=========================================================================

		// 标记资源在帧末提取（用于跨帧传递，如 TAA 历史缓冲）
		void QueueTextureExtraction(const std::string& Name, RGTextureHandle Handle);

		// 导入上帧提取的纹理
		RGTextureHandle ImportExtractedTexture(const std::string& Name);

		// 检查是否有已提取的纹理
		bool HasExtractedTexture(const std::string& Name) const;

		//=========================================================================
		// Present 布局转换 - 自动处理 Swapchain 图像
		//=========================================================================

		// 标记纹理需要在 Execute 结束后转换到 Present 布局
		void MarkForPresent(RGTextureHandle Handle);

		//=========================================================================
		// 资源查询
		//=========================================================================

		// 获取纹理描述符
		const RGTextureDesc* GetTextureDesc(RGTextureHandle Handle) const;

		// 获取缓冲区描述符
		const RGBufferDesc* GetBufferDesc(RGBufferHandle Handle) const;

		// 检查句柄是否有效
		bool IsValidHandle(RGResourceHandle Handle) const;

		// 获取 RHI 纹理（仅在执行阶段有效）
		Elaine::RHITexture* GetRHITexture(RGTextureHandle Handle) const;

		// 获取 RHI 缓冲区（仅在执行阶段有效）
		Elaine::RHIBuffer* GetRHIBuffer(RGBufferHandle Handle) const;

		//=========================================================================
		// Builder 访问
		//=========================================================================

		RenderGraphBuilder& GetBuilder() { return mBuilder; }

		// 资源池访问（用于外部代码创建持久资源）
		RenderGraphResourcePool& GetResourcePool() { return *mResourcePool; }

		//=========================================================================
		// 调试功能
		//=========================================================================

		// 启用验证
		void EnableValidation(bool Enable) { mValidationEnabled = Enable; }

		// 导出图结构（用于调试）
		void DumpGraph(const std::string& FilePath) const;

		// 获取统计信息
		struct Statistics
		{
			uint32 TotalPasses = 0;
			uint32 CulledPasses = 0;
			uint32 TotalTextures = 0;
			uint32 TotalBuffers = 0;
			uint32 TransientTextures = 0;
			uint32 TransientBuffers = 0;
			uint32 BarrierCount = 0;
			uint32 AsyncComputePasses = 0;
		};

		const Statistics& GetStatistics() const { return mStatistics; }

	private:
		//=========================================================================
		// 编译阶段
		//=========================================================================

		// 拓扑排序
		void TopologicalSort();

		// 资源生命周期分析
		void AnalyzeResourceLifetimes();

		// 死代码剔除
		void CullUnusedPasses();

		// 分配瞬态资源
		void AllocateTransientResources();

		// 插入资源屏障
		void InsertBarriers();

		// 计算异步计算同步点
		void ComputeAsyncSyncPoints();

		//=========================================================================
		// 执行阶段
		//=========================================================================

		// 执行单个 Pass
		void ExecutePass(CompiledPass& Pass, Elaine::RHICommandList* CmdList);

		// 执行资源屏障
		void ExecuteBarriers(const std::vector<RGBarrier>& Barriers, 
			Elaine::RHICommandList* CmdList);

		//=========================================================================
		// 资源管理
		//=========================================================================

		// 分配给定的资源描述符
		uint32 AllocateResourceIndex();

		// 清理当前帧数据
		void ClearFrameData();

		//=========================================================================
		// 数据成员
		//=========================================================================

		// Pass 存储
		std::vector<std::unique_ptr<RGPass>> mPasses;
		std::vector<CompiledPass> mCompiledPasses;
		std::vector<RGPass*> mSortedPasses;

		// 资源存储
		std::vector<std::unique_ptr<GraphResource>> mResources;
		std::vector<GraphTexture*> mTextures;
		std::vector<GraphBuffer*> mBuffers;
		std::unordered_map<std::string, RGResourceHandle> mResourceNameMap;

		// 异步计算同步
		std::vector<AsyncComputeSync> mAsyncSyncPoints;

		// 资源池
		std::unique_ptr<RenderGraphResourcePool> mResourcePool;

		// Extracted 资源存储（跨帧持久）
		struct ExtractedTexture
		{
			Elaine::RHITexture* Texture = nullptr;
			RGTextureDesc Desc;
		};
		std::unordered_map<std::string, ExtractedTexture> mExtractedTextures;
		std::vector<std::pair<std::string, RGTextureHandle>> mPendingExtractions;

		// Present 转换队列（Execute 结束后自动插入屏障）
		std::vector<RGTextureHandle> mPresentTransitions;

		// 构建器
		RenderGraphBuilder mBuilder;

		// 状态
		bool mIsCompiled = false;
		bool mValidationEnabled = false;
		bool mIsExecuting = false;

		// 统计信息
		Statistics mStatistics;

		// 当前资源索引
		uint32 mNextResourceIndex = 0;
	};
}