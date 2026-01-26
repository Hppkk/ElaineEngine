#pragma once
#include "ElaineCorePrerequirements.h"
#include "RenderGraph/ElaineRenderGraphResource.h"
#include <vector>
#include <unordered_map>

namespace Elaine
{
	class RHITexture;
	class RHIBuffer;
	class RHICommandContext;
}

namespace RenderGraph
{
	//=============================================================================
	// 资源池入口
	//=============================================================================
	struct PooledTexture
	{
		Elaine::RHITexture* Texture = nullptr;
		RGTextureDesc Desc;
		std::string Name;								// 用于 Persistent 资源的名称匹配
		uint32 FramesSinceLastUse = 0;
		ERGResourceState Lifetime = ERGResourceState::Transient;
		bool InUse = false;
	};

	struct PooledBuffer
	{
		Elaine::RHIBuffer* Buffer = nullptr;
		RGBufferDesc Desc;
		std::string Name;								// 用于 Persistent 资源的名称匹配
		uint32 FramesSinceLastUse = 0;
		ERGResourceState Lifetime = ERGResourceState::Transient;
		bool InUse = false;
	};

	//=============================================================================
	// RenderGraphResourcePool - 资源池，用于跨帧的 RHI 资源复用
	//=============================================================================
	class ElaineCoreExport RenderGraphResourcePool
	{
	public:
		RenderGraphResourcePool();
		~RenderGraphResourcePool();

		// 从池中获取或创建瞬态纹理
		Elaine::RHITexture* AcquireTexture(const RGTextureDesc& Desc, 
			Elaine::RHICommandContext* Context);

		// 获取或创建持久纹理（按名称匹配，常驻内存）
		Elaine::RHITexture* AcquirePersistentTexture(const std::string& Name,
			const RGTextureDesc& Desc, Elaine::RHICommandContext* Context);

		// 释放纹理回池中
		void ReleaseTexture(Elaine::RHITexture* Texture);

		// 从池中获取或创建瞬态缓冲区
		Elaine::RHIBuffer* AcquireBuffer(const RGBufferDesc& Desc,
			Elaine::RHICommandContext* Context);

		// 获取或创建持久缓冲区（按名称匹配，常驻内存）
		Elaine::RHIBuffer* AcquirePersistentBuffer(const std::string& Name,
			const RGBufferDesc& Desc, Elaine::RHICommandContext* Context);

		// 释放缓冲区回池中
		void ReleaseBuffer(Elaine::RHIBuffer* Buffer);

		// 每帧开始时调用，重置所有资源的使用状态
		void BeginFrame();

		// 每帧调用，更新资源年龄并清理过期资源
		void Tick();

		// 清空所有池中资源
		void Clear();

		// 设置资源过期帧数（超过此帧数未使用将被释放）
		void SetExpirationFrames(uint32 Frames) { mExpirationFrames = Frames; }

		//=========================================================================
		// 统计信息
		//=========================================================================

		struct PoolStatistics
		{
			uint32 TexturesInPool = 0;
			uint32 TexturesInUse = 0;
			uint32 BuffersInPool = 0;
			uint32 BuffersInUse = 0;
			size_t TotalTextureMemory = 0;
			size_t TotalBufferMemory = 0;
		};

		PoolStatistics GetStatistics() const;

	private:
		// 检查描述符是否兼容
		bool IsTextureCompatible(const PooledTexture& Pooled, const RGTextureDesc& Desc) const;
		bool IsBufferCompatible(const PooledBuffer& Pooled, const RGBufferDesc& Desc) const;

		// 创建 RHI 资源
		Elaine::RHITexture* CreateRHITexture(const RGTextureDesc& Desc, 
			Elaine::RHICommandContext* Context);
		Elaine::RHIBuffer* CreateRHIBuffer(const RGBufferDesc& Desc,
			Elaine::RHICommandContext* Context);

		// 释放 RHI 资源
		void DestroyRHITexture(Elaine::RHITexture* Texture);
		void DestroyRHIBuffer(Elaine::RHIBuffer* Buffer);

		//=========================================================================
		// 数据成员
		//=========================================================================

		std::vector<PooledTexture> mTexturePool;
		std::vector<PooledBuffer> mBufferPool;

		// 用于快速查找纹理
		std::unordered_map<Elaine::RHITexture*, uint32> mTextureToIndex;
		std::unordered_map<Elaine::RHIBuffer*, uint32> mBufferToIndex;

		uint32 mExpirationFrames = 3; // 默认 3 帧后过期
	};
}
