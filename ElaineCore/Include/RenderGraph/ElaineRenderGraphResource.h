#pragma once
#include "ElaineCorePrerequirements.h"
#include "common/ElaineRHIProtocol.h"
#include <string>
#include <vector>

namespace Elaine
{
	class RHIResource;
	class RHITexture;
	class RHIBuffer;
}

namespace RenderGraph
{
	//=============================================================================
	// 前向声明
	//=============================================================================
	class RGPass;
	class RenderGraphBuilder;
	class RenderDependencyGraph;

	//=============================================================================
	// 资源句柄 - 用于引用图中的虚拟资源
	//=============================================================================
	struct RGResourceHandle
	{
		uint32 Index = UINT32_MAX;
		uint32 Version = 0;

		bool IsValid() const { return Index != UINT32_MAX; }
		bool operator==(const RGResourceHandle& Other) const
		{
			return Index == Other.Index && Version == Other.Version;
		}
		bool operator!=(const RGResourceHandle& Other) const
		{
			return !(*this == Other);
		}
	};

	using RGTextureHandle = RGResourceHandle;
	using RGBufferHandle = RGResourceHandle;

	//=============================================================================
	// 资源访问类型
	//=============================================================================
	enum class ERGResourceAccess : uint8
	{
		None = 0,
		Read = 1 << 0,
		Write = 1 << 1,
		ReadWrite = Read | Write
	};

	inline ERGResourceAccess operator|(ERGResourceAccess A, ERGResourceAccess B)
	{
		return static_cast<ERGResourceAccess>(static_cast<uint8>(A) | static_cast<uint8>(B));
	}

	inline ERGResourceAccess operator&(ERGResourceAccess A, ERGResourceAccess B)
	{
		return static_cast<ERGResourceAccess>(static_cast<uint8>(A) & static_cast<uint8>(B));
	}

	//=============================================================================
	// Pass 队列类型 - 支持异步计算
	//=============================================================================
	enum class ERGQueueType : uint8
	{
		Graphics = 0,	// 图形队列
		Compute,		// 异步计算队列
		Copy,			// 拷贝队列
		Count
	};

	//=============================================================================
	// 纹理描述符
	//=============================================================================
	struct RGTextureDesc
	{
		uint32 Width = 0;
		uint32 Height = 0;
		uint32 Depth = 1;
		uint32 MipLevels = 1;
		uint32 ArraySize = 1;
		PixelFormat Format = PF_Unknown;
		TextureCreateFlags Flags = TextureCreateFlags::None;
		ERHIAccess InitialState = ERHIAccess::None;
		LinearColor ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		uint32 NumSamples = 1;

		// 辅助方法
		bool IsRenderTarget() const
		{
			return (static_cast<uint64>(Flags) & static_cast<uint64>(TextureCreateFlags::RenderTargetable)) != 0;
		}

		bool IsDepthStencil() const
		{
			return (static_cast<uint64>(Flags) & static_cast<uint64>(TextureCreateFlags::DepthStencilTargetable)) != 0;
		}

		bool IsUAV() const
		{
			return (static_cast<uint64>(Flags) & static_cast<uint64>(TextureCreateFlags::UAV)) != 0;
		}

		static RGTextureDesc Create2D(uint32 InWidth, uint32 InHeight, PixelFormat InFormat,
			TextureCreateFlags InFlags = TextureCreateFlags::ShaderResource)
		{
			RGTextureDesc Desc;
			Desc.Width = InWidth;
			Desc.Height = InHeight;
			Desc.Format = InFormat;
			Desc.Flags = InFlags;
			return Desc;
		}
	};

	//=============================================================================
	// 缓冲区描述符
	//=============================================================================
	struct RGBufferDesc
	{
		uint32 Size = 0;
		uint32 Stride = 0;
		BufferUsageFlags Usage = BufferUsageFlags::None;
		ERHIAccess InitialState = ERHIAccess::None;

		static RGBufferDesc CreateStructured(uint32 InStride, uint32 InCount, BufferUsageFlags InUsage = BufferUsageFlags::None)
		{
			RGBufferDesc Desc;
			Desc.Stride = InStride;
			Desc.Size = InStride * InCount;
			Desc.Usage = InUsage;
			return Desc;
		}
	};

	//=============================================================================
	// 资源别名视图 - 支持同一资源以不同格式访问
	//=============================================================================
	struct RGResourceAlias
	{
		RGResourceHandle SourceHandle;
		PixelFormat ViewFormat = PF_Unknown;	// 重新解释的格式
		uint32 MipLevel = 0;
		uint32 ArraySlice = 0;
		uint32 MipCount = 1;
		uint32 ArrayCount = 1;
	};

	//=============================================================================
	// 资源状态 - 用于跟踪资源在图中的状态
	//=============================================================================
	enum class ERGResourceState : uint8
	{
		Uninitialized,	// 未初始化
		Transient,		// 瞬态资源（每帧分配/释放）
		Persistent,		// 持久资源（常驻内存，按名称复用）
		External,		// 外部导入资源
		Extracted		// 已导出资源（跨帧存在）
	};

	//=============================================================================
	// 资源生命周期信息
	//=============================================================================
	struct RGResourceLifetime
	{
		uint32 FirstPassIndex = UINT32_MAX;	// 首次使用的 Pass 索引
		uint32 LastPassIndex = 0;				// 最后使用的 Pass 索引
		ERGResourceAccess AccessFlags = ERGResourceAccess::None;

		bool IsUsed() const { return FirstPassIndex != UINT32_MAX; }
	};

	//=============================================================================
	// 图资源基类
	//=============================================================================
	class ElaineCoreExport GraphResource
	{
	public:
		GraphResource() = default;
		explicit GraphResource(const std::string& InName);
		virtual ~GraphResource();

		// 获取 RHI 资源（仅在执行阶段有效）
		RHIResource* GetRHIHandle() const { return mResourceRHI; }

		// 资源标识
		const std::string& GetName() const { return mName; }
		RGResourceHandle GetHandle() const { return mHandle; }
		void SetHandle(RGResourceHandle InHandle) { mHandle = InHandle; }

		// 状态管理
		ERGResourceState GetState() const { return mState; }
		void SetState(ERGResourceState InState) { mState = InState; }

		// 生命周期
		const RGResourceLifetime& GetLifetime() const { return mLifetime; }
		RGResourceLifetime& GetLifetimeMutable() { return mLifetime; }

		// 引用计数（用于剔除优化）
		void AddRef() { ++mRefCount; }
		void Release() { if (mRefCount > 0) --mRefCount; }
		uint32 GetRefCount() const { return mRefCount; }

		// 是否可被剔除
		bool CanBeCulled() const { return mRefCount == 0 && !mNeverCull; }
		void SetNeverCull(bool Value) { mNeverCull = Value; }

		// 类型检查
		virtual bool IsTexture() const { return false; }
		virtual bool IsBuffer() const { return false; }

	protected:
		std::string mName;
		RGResourceHandle mHandle;
		ERGResourceState mState = ERGResourceState::Uninitialized;
		RGResourceLifetime mLifetime;
		RHIResource* mResourceRHI = nullptr;
		uint32 mRefCount = 0;
		bool mNeverCull = false;
	};

	//=============================================================================
	// 图纹理资源
	//=============================================================================
	class ElaineCoreExport GraphTexture : public GraphResource
	{
	public:
		GraphTexture() = default;
		GraphTexture(const std::string& InName, const RGTextureDesc& InDesc);
		virtual ~GraphTexture() override;

		bool IsTexture() const override { return true; }

		// 描述符
		const RGTextureDesc& GetDesc() const { return mDesc; }
		void SetDesc(const RGTextureDesc& InDesc) { mDesc = InDesc; }

		// RHI 纹理访问
		RHITexture* GetRHITexture() const;
		void SetRHITexture(RHITexture* InTexture);

		// 资源别名支持
		bool HasAlias() const { return mAlias.SourceHandle.IsValid(); }
		const RGResourceAlias& GetAlias() const { return mAlias; }
		void SetAlias(const RGResourceAlias& InAlias) { mAlias = InAlias; }

	private:
		RGTextureDesc mDesc;
		RGResourceAlias mAlias;
	};

	//=============================================================================
	// 图缓冲区资源
	//=============================================================================
	class ElaineCoreExport GraphBuffer : public GraphResource
	{
	public:
		GraphBuffer() = default;
		GraphBuffer(const std::string& InName, const RGBufferDesc& InDesc);
		virtual ~GraphBuffer() override;

		bool IsBuffer() const override { return true; }

		// 描述符
		const RGBufferDesc& GetDesc() const { return mDesc; }
		void SetDesc(const RGBufferDesc& InDesc) { mDesc = InDesc; }

		// RHI 缓冲区访问
		RHIBuffer* GetRHIBuffer() const;
		void SetRHIBuffer(RHIBuffer* InBuffer);

	private:
		RGBufferDesc mDesc;
	};
}