#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderGraph/ElaineRenderGraphResource.h"
#include "RenderGraph/ElaineRenderPass.h"
#include <functional>
#include <memory>
#include <string>

namespace Elaine
{
	class RHITexture;
	class RHIBuffer;
}

namespace RenderGraph
{
	class RenderDependencyGraph;

	//=============================================================================
	// RenderGraphBuilder - 用于在 Pass Setup 阶段声明资源和依赖
	//=============================================================================
	class ElaineCoreExport RenderGraphBuilder
	{
	public:
		RenderGraphBuilder();
		explicit RenderGraphBuilder(RenderDependencyGraph* Graph);
		~RenderGraphBuilder();

		// 设置当前正在构建的 Pass
		void SetCurrentPass(RGPass* Pass) { mCurrentPass = Pass; }
		RGPass* GetCurrentPass() const { return mCurrentPass; }

		//=========================================================================
		// 资源创建
		//=========================================================================

		// 创建瞬态纹理（每帧自动分配/释放）
		RGTextureHandle CreateTexture(const std::string& Name, const RGTextureDesc& Desc);

		// 创建瞬态缓冲区
		RGBufferHandle CreateBuffer(const std::string& Name, const RGBufferDesc& Desc);

		// 创建持久纹理（常驻内存，不参与帧末重置）
		RGTextureHandle CreatePersistentTexture(const std::string& Name, const RGTextureDesc& Desc);

		// 创建持久缓冲区
		RGBufferHandle CreatePersistentBuffer(const std::string& Name, const RGBufferDesc& Desc);

		//=========================================================================
		// 外部资源导入
		//=========================================================================

		// 导入外部纹理（如交换链、持久化纹理）
		RGTextureHandle ImportTexture(const std::string& Name, Elaine::RHITexture* Texture,
			const RGTextureDesc& Desc = {});

		// 导入外部缓冲区
		RGBufferHandle ImportBuffer(const std::string& Name, Elaine::RHIBuffer* Buffer,
			const RGBufferDesc& Desc = {});

		// 导入上帧提取的纹理
		RGTextureHandle ImportExtractedTexture(const std::string& Name);

		// 检查是否有已提取的纹理
		bool HasExtractedTexture(const std::string& Name) const;

		// 标记纹理在帧末提取（用于跨帧传递，如 TAA 历史缓冲）
		void QueueTextureExtraction(const std::string& Name, RGTextureHandle Handle);

		// 标记纹理需要在 Execute 结束后转换到 Present 布局
		void MarkForPresent(RGTextureHandle Handle);

		//=========================================================================
		// 资源访问声明
		//=========================================================================

		// 声明读取纹理
		RGTextureHandle ReadTexture(RGTextureHandle Handle,
			Elaine::ERHIAccess ReadAccess = Elaine::ERHIAccess::SRVGraphics);

		// 声明写入纹理
		RGTextureHandle WriteTexture(RGTextureHandle Handle,
			Elaine::ERHIAccess WriteAccess = Elaine::ERHIAccess::RTV);

		// 声明读写纹理（UAV）
		RGTextureHandle ReadWriteTexture(RGTextureHandle Handle,
			Elaine::ERHIAccess Access = Elaine::ERHIAccess::UAVGraphics);

		// 声明读取缓冲区
		RGBufferHandle ReadBuffer(RGBufferHandle Handle,
			Elaine::ERHIAccess ReadAccess = Elaine::ERHIAccess::SRVGraphics);

		// 声明写入缓冲区
		RGBufferHandle WriteBuffer(RGBufferHandle Handle,
			Elaine::ERHIAccess WriteAccess = Elaine::ERHIAccess::UAVGraphics);

		// 声明读写缓冲区
		RGBufferHandle ReadWriteBuffer(RGBufferHandle Handle,
			Elaine::ERHIAccess Access = Elaine::ERHIAccess::UAVGraphics);

		//=========================================================================
		// 资源别名 - 以不同格式/视图读取同一资源
		//=========================================================================

		// 创建纹理别名视图
		RGTextureHandle CreateTextureAlias(RGTextureHandle SourceHandle,
			const std::string& AliasName,
			Elaine::PixelFormat ViewFormat,
			uint32 MipLevel = 0, uint32 ArraySlice = 0);

		//=========================================================================
		// 渲染目标设置
		//=========================================================================

		// 设置颜色渲染目标
		void SetRenderTarget(uint32 Index, RGTextureHandle Texture,
			const RGRenderTargetDesc& Desc = {});

		// 设置深度模板目标
		void SetDepthStencil(RGTextureHandle Texture,
			const RGDepthStencilDesc& Desc = {});

		//=========================================================================
		// Pass 创建 - 类型安全的模板方法
		//=========================================================================

		template<typename PassDataType, typename SetupFunc, typename ExecuteFunc>
		PassDataType& AddPass(const std::string& Name, PassType Type,
			SetupFunc&& Setup, ExecuteFunc&& Execute);

		// Raster Pass 快捷方法
		template<typename PassDataType, typename SetupFunc, typename ExecuteFunc>
		PassDataType& AddRasterPass(const std::string& Name,
			SetupFunc&& Setup, ExecuteFunc&& Execute)
		{
			return AddPass<PassDataType>(Name, PassType::Raster,
				std::forward<SetupFunc>(Setup), std::forward<ExecuteFunc>(Execute));
		}

		// Compute Pass 快捷方法
		template<typename PassDataType, typename SetupFunc, typename ExecuteFunc>
		PassDataType& AddComputePass(const std::string& Name,
			SetupFunc&& Setup, ExecuteFunc&& Execute)
		{
			return AddPass<PassDataType>(Name, PassType::Compute,
				std::forward<SetupFunc>(Setup), std::forward<ExecuteFunc>(Execute));
		}

		// Async Compute Pass 快捷方法
		template<typename PassDataType, typename SetupFunc, typename ExecuteFunc>
		PassDataType& AddAsyncComputePass(const std::string& Name,
			SetupFunc&& Setup, ExecuteFunc&& Execute)
		{
			return AddPass<PassDataType>(Name, PassType::AsyncCompute,
				std::forward<SetupFunc>(Setup), std::forward<ExecuteFunc>(Execute));
		}

		// Copy Pass 快捷方法
		template<typename PassDataType, typename SetupFunc, typename ExecuteFunc>
		PassDataType& AddCopyPass(const std::string& Name,
			SetupFunc&& Setup, ExecuteFunc&& Execute)
		{
			return AddPass<PassDataType>(Name, PassType::Copy,
				std::forward<SetupFunc>(Setup), std::forward<ExecuteFunc>(Execute));
		}

		//=========================================================================
		// 查询方法
		//=========================================================================

		// 获取纹理描述符
		const RGTextureDesc* GetTextureDesc(RGTextureHandle Handle) const;

		// 获取缓冲区描述符
		const RGBufferDesc* GetBufferDesc(RGBufferHandle Handle) const;

		// 检查资源是否存在
		bool IsValidHandle(RGResourceHandle Handle) const;

	private:
		RenderDependencyGraph* mGraph = nullptr;
		RGPass* mCurrentPass = nullptr;

		// 内部辅助方法
		void AddPassToGraph(std::unique_ptr<RGPass> Pass);
	};

	//=============================================================================
	// AddPass 模板实现
	//=============================================================================
	template<typename PassDataType, typename SetupFunc, typename ExecuteFunc>
	PassDataType& RenderGraphBuilder::AddPass(const std::string& Name, PassType Type,
		SetupFunc&& Setup, ExecuteFunc&& Execute)
	{
		// 创建类型化的 Pass
		auto Pass = std::make_unique<TRGPass<PassDataType>>(Name, Type);
		PassDataType& Data = Pass->GetData();

		// 设置当前 Pass 以便在 Setup 中使用
		RGPass* PreviousPass = mCurrentPass;
		mCurrentPass = Pass.get();

		// 调用 Setup 回调
		Setup(*this, Data);

		// 设置执行回调
		Pass->SetTypedExecuteCallback(std::forward<ExecuteFunc>(Execute));

		// 恢复之前的 Pass
		mCurrentPass = PreviousPass;

		// 将 Pass 添加到图中
		if (mGraph)
		{
			AddPassToGraph(std::move(Pass));
		}

		return Data;
	}
}