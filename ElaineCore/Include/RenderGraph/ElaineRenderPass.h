#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderGraph/ElaineRenderGraphResource.h"
#include <functional>
#include <memory>
#include <vector>

namespace Elaine
{
	class RHICommandContext;
	class RHICommandList;
}

namespace RenderGraph
{
	class RenderGraphBuilder;
	class RenderDependencyGraph;

	//=============================================================================
	// Pass 类型
	//=============================================================================
	enum class PassType : uint8
	{
		None = 0,
		Raster,			// 光栅化 Pass
		Compute,		// 计算 Pass（图形队列）
		AsyncCompute,	// 异步计算 Pass（计算队列）
		Copy			// 拷贝 Pass
	};

	//=============================================================================
	// Pass 标志
	//=============================================================================
	enum class PassFlags : uint32
	{
		None = 0,
		NeverCull = 1 << 0,			// 禁止被剔除
		SkipRenderPass = 1 << 1,	// 跳过硬件 RenderPass（例如仅设置状态）
		AsyncCompute = 1 << 2,		// 在异步计算队列执行
		Copy = 1 << 3				// 在拷贝队列执行
	};

	inline PassFlags operator|(PassFlags A, PassFlags B)
	{
		return static_cast<PassFlags>(static_cast<uint32>(A) | static_cast<uint32>(B));
	}

	inline bool HasFlag(PassFlags Flags, PassFlags Flag)
	{
		return (static_cast<uint32>(Flags) & static_cast<uint32>(Flag)) != 0;
	}

	//=============================================================================
	// 渲染目标描述
	//=============================================================================
	struct RGRenderTargetDesc
	{
		uint32 MipLevel = 0;
		uint32 ArraySlice = 0;
		ERenderTargetActions LoadStoreOp = ERenderTargetActions::DontLoad_Store;
		Elaine::LinearColor ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	};

	//=============================================================================
	// 深度模板描述
	//=============================================================================
	struct RGDepthStencilDesc
	{
		uint32 MipLevel = 0;
		uint32 ArraySlice = 0;
		EDepthStencilTargetActions LoadStoreOp = EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil;
		float ClearDepth = 1.0f;
		uint32 ClearStencil = 0;
		bool ReadOnly = false;
	};

	//=============================================================================
	// 资源使用信息
	//=============================================================================
	struct RGResourceUsage
	{
		RGResourceHandle Handle;
		ERGResourceAccess Access = ERGResourceAccess::None;
		Elaine::ERHIAccess RHIState = Elaine::ERHIAccess::None;
	};

	//=============================================================================
	// RenderPass 基类
	//=============================================================================
	class ElaineCoreExport RGPass
	{
	public:
		using ExecuteCallback = std::function<void(Elaine::RHICommandList*)>;

		RGPass() = default;
		explicit RGPass(const std::string& InName, PassType InType = PassType::None);
		virtual ~RGPass();

		// 执行
		virtual void Execute(Elaine::RHICommandList* CmdList);

		// 基本信息
		const std::string& GetName() const { return mName; }
		PassType GetType() const { return mPassType; }
		PassFlags GetFlags() const { return mFlags; }
		void SetFlags(PassFlags InFlags) { mFlags = InFlags; }
		uint32 GetIndex() const { return mIndex; }
		void SetIndex(uint32 InIndex) { mIndex = InIndex; }

		// 资源依赖
		void AddTextureInput(RGTextureHandle Handle, ERGResourceAccess Access, Elaine::ERHIAccess State);
		void AddTextureOutput(RGTextureHandle Handle, Elaine::ERHIAccess State);
		void AddBufferInput(RGBufferHandle Handle, ERGResourceAccess Access, Elaine::ERHIAccess State);
		void AddBufferOutput(RGBufferHandle Handle, Elaine::ERHIAccess State);

		const std::vector<RGResourceUsage>& GetTextureInputs() const { return mTextureInputs; }
		const std::vector<RGResourceUsage>& GetTextureOutputs() const { return mTextureOutputs; }
		const std::vector<RGResourceUsage>& GetBufferInputs() const { return mBufferInputs; }
		const std::vector<RGResourceUsage>& GetBufferOutputs() const { return mBufferOutputs; }

		// 渲染目标
		void SetRenderTarget(uint32 Index, RGTextureHandle Handle, const RGRenderTargetDesc& Desc = {});
		void SetDepthStencil(RGTextureHandle Handle, const RGDepthStencilDesc& Desc = {});

		struct RenderTargetBinding
		{
			RGTextureHandle Handle;
			RGRenderTargetDesc Desc;
		};

		struct DepthStencilBinding
		{
			RGTextureHandle Handle;
			RGDepthStencilDesc Desc;
		};

		const std::vector<RenderTargetBinding>& GetRenderTargets() const { return mRenderTargets; }
		const DepthStencilBinding* GetDepthStencil() const { return mHasDepthStencil ? &mDepthStencil : nullptr; }

		// 队列类型
		ERGQueueType GetQueueType() const;

		// 剔除相关
		bool CanBeCulled() const { return !HasFlag(mFlags, PassFlags::NeverCull) && mRefCount == 0; }
		void AddRef() { ++mRefCount; }
		void Release() { if (mRefCount > 0) --mRefCount; }

		// 执行回调
		void SetExecuteCallback(ExecuteCallback Callback) { mExecuteCallback = std::move(Callback); }

	protected:
		std::string mName;
		PassType mPassType = PassType::None;
		PassFlags mFlags = PassFlags::None;
		uint32 mIndex = 0;
		uint32 mRefCount = 0;

		// 资源使用
		std::vector<RGResourceUsage> mTextureInputs;
		std::vector<RGResourceUsage> mTextureOutputs;
		std::vector<RGResourceUsage> mBufferInputs;
		std::vector<RGResourceUsage> mBufferOutputs;

		// 渲染目标
		std::vector<RenderTargetBinding> mRenderTargets;
		DepthStencilBinding mDepthStencil;
		bool mHasDepthStencil = false;

		// 执行回调
		ExecuteCallback mExecuteCallback;
	};

	//=============================================================================
	// 类型安全的 Pass 数据模板
	//=============================================================================
	template<typename PassDataType>
	class TRGPass : public RGPass
	{
	public:
		using SetupCallback = std::function<void(RenderGraphBuilder&, PassDataType&)>;
		using TypedExecuteCallback = std::function<void(Elaine::RHICommandList*, const PassDataType&)>;

		TRGPass(const std::string& InName, PassType InType)
			: RGPass(InName, InType)
		{
		}

		PassDataType& GetData() { return mPassData; }
		const PassDataType& GetData() const { return mPassData; }

		void SetTypedExecuteCallback(TypedExecuteCallback Callback)
		{
			mTypedExecuteCallback = std::move(Callback);
			// 包装为通用回调
			SetExecuteCallback([this](Elaine::RHICommandList* CmdList) {
				if (mTypedExecuteCallback)
				{
					mTypedExecuteCallback(CmdList, mPassData);
				}
			});
		}

	private:
		PassDataType mPassData;
		TypedExecuteCallback mTypedExecuteCallback;
	};
}