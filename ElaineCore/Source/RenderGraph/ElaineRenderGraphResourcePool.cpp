#include "ElainePrecompiledHeader.h"
#include "RenderGraph/ElaineRenderGraphResourcePool.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHITypes.h"

namespace RenderGraph
{
	//=============================================================================
	// 构造/析构
	//=============================================================================
	RenderGraphResourcePool::RenderGraphResourcePool()
	{
	}

	RenderGraphResourcePool::~RenderGraphResourcePool()
	{
		Clear();
	}

	//=============================================================================
	// 纹理管理
	//=============================================================================
	Elaine::RHITexture* RenderGraphResourcePool::AcquireTexture(const RGTextureDesc& Desc,
		Elaine::RHICommandContext* Context)
	{
		// 查找兼容的空闲纹理
		for (auto& Pooled : mTexturePool)
		{
			if (!Pooled.InUse && IsTextureCompatible(Pooled, Desc))
			{
				Pooled.InUse = true;
				Pooled.FramesSinceLastUse = 0;
				return Pooled.Texture;
			}
		}

		// 没有找到，创建新的
		Elaine::RHITexture* NewTexture = CreateRHITexture(Desc, Context);
		if (NewTexture)
		{
			PooledTexture Pooled;
			Pooled.Texture = NewTexture;
			Pooled.Desc = Desc;
			Pooled.InUse = true;
			Pooled.FramesSinceLastUse = 0;

			mTextureToIndex[NewTexture] = static_cast<uint32>(mTexturePool.size());
			mTexturePool.push_back(Pooled);
		}

		return NewTexture;
	}

	void RenderGraphResourcePool::ReleaseTexture(Elaine::RHITexture* Texture)
	{
		if (!Texture)
			return;

		auto It = mTextureToIndex.find(Texture);
		if (It != mTextureToIndex.end())
		{
			mTexturePool[It->second].InUse = false;
		}
	}

	//=============================================================================
	// 缓冲区管理
	//=============================================================================
	Elaine::RHIBuffer* RenderGraphResourcePool::AcquireBuffer(const RGBufferDesc& Desc,
		Elaine::RHICommandContext* Context)
	{
		// 查找兼容的空闲缓冲区
		for (auto& Pooled : mBufferPool)
		{
			if (!Pooled.InUse && IsBufferCompatible(Pooled, Desc))
			{
				Pooled.InUse = true;
				Pooled.FramesSinceLastUse = 0;
				return Pooled.Buffer;
			}
		}

		// 创建新的
		Elaine::RHIBuffer* NewBuffer = CreateRHIBuffer(Desc, Context);
		if (NewBuffer)
		{
			PooledBuffer Pooled;
			Pooled.Buffer = NewBuffer;
			Pooled.Desc = Desc;
			Pooled.InUse = true;
			Pooled.FramesSinceLastUse = 0;

			mBufferToIndex[NewBuffer] = static_cast<uint32>(mBufferPool.size());
			mBufferPool.push_back(Pooled);
		}

		return NewBuffer;
	}

	void RenderGraphResourcePool::ReleaseBuffer(Elaine::RHIBuffer* Buffer)
	{
		if (!Buffer)
			return;

		auto It = mBufferToIndex.find(Buffer);
		if (It != mBufferToIndex.end())
		{
			mBufferPool[It->second].InUse = false;
		}
	}

	Elaine::RHITexture* RenderGraphResourcePool::AcquirePersistentTexture(const std::string& Name,
		const RGTextureDesc& Desc, Elaine::RHICommandContext* Context)
	{
		// 按名称查找已有的持久资源
		for (auto& Pooled : mTexturePool)
		{
			if (Pooled.Lifetime == ERGResourceState::Persistent && Pooled.Name == Name)
			{
				Pooled.InUse = true;
				Pooled.FramesSinceLastUse = 0;
				return Pooled.Texture;
			}
		}

		// 创建新的持久资源
		Elaine::RHITexture* NewTexture = CreateRHITexture(Desc, Context);
		if (NewTexture)
		{
			PooledTexture Pooled;
			Pooled.Texture = NewTexture;
			Pooled.Desc = Desc;
			Pooled.Name = Name;
			Pooled.Lifetime = ERGResourceState::Persistent;
			Pooled.InUse = true;
			Pooled.FramesSinceLastUse = 0;

			mTextureToIndex[NewTexture] = static_cast<uint32>(mTexturePool.size());
			mTexturePool.push_back(Pooled);
		}
		return NewTexture;
	}

	Elaine::RHIBuffer* RenderGraphResourcePool::AcquirePersistentBuffer(const std::string& Name,
		const RGBufferDesc& Desc, Elaine::RHICommandContext* Context)
	{
		// 按名称查找已有的持久资源
		for (auto& Pooled : mBufferPool)
		{
			if (Pooled.Lifetime == ERGResourceState::Persistent && Pooled.Name == Name)
			{
				Pooled.InUse = true;
				Pooled.FramesSinceLastUse = 0;
				return Pooled.Buffer;
			}
		}

		// 创建新的持久资源
		Elaine::RHIBuffer* NewBuffer = CreateRHIBuffer(Desc, Context);
		if (NewBuffer)
		{
			PooledBuffer Pooled;
			Pooled.Buffer = NewBuffer;
			Pooled.Desc = Desc;
			Pooled.Name = Name;
			Pooled.Lifetime = ERGResourceState::Persistent;
			Pooled.InUse = true;
			Pooled.FramesSinceLastUse = 0;

			mBufferToIndex[NewBuffer] = static_cast<uint32>(mBufferPool.size());
			mBufferPool.push_back(Pooled);
		}
		return NewBuffer;
	}

	//=============================================================================
	// 生命周期管理
	//=============================================================================
	void RenderGraphResourcePool::BeginFrame()
	{
		// 只重置 Transient 资源的使用状态，Persistent 资源保持已有状态
		for (auto& Pooled : mTexturePool)
		{
			if (Pooled.Lifetime == ERGResourceState::Transient)
			{
				Pooled.InUse = false;
			}
		}
		for (auto& Pooled : mBufferPool)
		{
			if (Pooled.Lifetime == ERGResourceState::Transient)
			{
				Pooled.InUse = false;
			}
		}
	}

	void RenderGraphResourcePool::Tick()
	{
		// 更新纹理年龄并清理过期的
		for (size_t i = 0; i < mTexturePool.size(); )
		{
			auto& Pooled = mTexturePool[i];
			if (!Pooled.InUse)
			{
				Pooled.FramesSinceLastUse++;
				if (Pooled.FramesSinceLastUse > mExpirationFrames)
				{
					// 释放过期资源
					mTextureToIndex.erase(Pooled.Texture);
					DestroyRHITexture(Pooled.Texture);

					// 移除条目（与最后一个交换）
					if (i != mTexturePool.size() - 1)
					{
						mTextureToIndex[mTexturePool.back().Texture] = static_cast<uint32>(i);
						std::swap(mTexturePool[i], mTexturePool.back());
					}
					mTexturePool.pop_back();
					continue;
				}
			}
			++i;
		}

		// 更新缓冲区年龄并清理过期的
		for (size_t i = 0; i < mBufferPool.size(); )
		{
			auto& Pooled = mBufferPool[i];
			if (!Pooled.InUse)
			{
				Pooled.FramesSinceLastUse++;
				if (Pooled.FramesSinceLastUse > mExpirationFrames)
				{
					mBufferToIndex.erase(Pooled.Buffer);
					DestroyRHIBuffer(Pooled.Buffer);

					if (i != mBufferPool.size() - 1)
					{
						mBufferToIndex[mBufferPool.back().Buffer] = static_cast<uint32>(i);
						std::swap(mBufferPool[i], mBufferPool.back());
					}
					mBufferPool.pop_back();
					continue;
				}
			}
			++i;
		}
	}

	void RenderGraphResourcePool::Clear()
	{
		for (auto& Pooled : mTexturePool)
		{
			DestroyRHITexture(Pooled.Texture);
		}
		mTexturePool.clear();
		mTextureToIndex.clear();

		for (auto& Pooled : mBufferPool)
		{
			DestroyRHIBuffer(Pooled.Buffer);
		}
		mBufferPool.clear();
		mBufferToIndex.clear();
	}

	//=============================================================================
	// 统计信息
	//=============================================================================
	RenderGraphResourcePool::PoolStatistics RenderGraphResourcePool::GetStatistics() const
	{
		PoolStatistics Stats;
		Stats.TexturesInPool = static_cast<uint32>(mTexturePool.size());
		Stats.BuffersInPool = static_cast<uint32>(mBufferPool.size());

		for (const auto& Pooled : mTexturePool)
		{
			if (Pooled.InUse)
				Stats.TexturesInUse++;
			// 估算纹理内存（简化计算）
			Stats.TotalTextureMemory += static_cast<size_t>(Pooled.Desc.Width) * Pooled.Desc.Height * 4;
		}

		for (const auto& Pooled : mBufferPool)
		{
			if (Pooled.InUse)
				Stats.BuffersInUse++;
			Stats.TotalBufferMemory += Pooled.Desc.Size;
		}

		return Stats;
	}

	//=============================================================================
	// 内部辅助
	//=============================================================================
	bool RenderGraphResourcePool::IsTextureCompatible(const PooledTexture& Pooled,
		const RGTextureDesc& Desc) const
	{
		// 精确匹配尺寸和格式
		return Pooled.Desc.Width == Desc.Width &&
			Pooled.Desc.Height == Desc.Height &&
			Pooled.Desc.Depth == Desc.Depth &&
			Pooled.Desc.MipLevels == Desc.MipLevels &&
			Pooled.Desc.ArraySize == Desc.ArraySize &&
			Pooled.Desc.Format == Desc.Format &&
			Pooled.Desc.Flags == Desc.Flags &&
			Pooled.Desc.NumSamples == Desc.NumSamples;
	}

	bool RenderGraphResourcePool::IsBufferCompatible(const PooledBuffer& Pooled,
		const RGBufferDesc& Desc) const
	{
		// 缓冲区需要满足大小要求
		return Pooled.Desc.Size >= Desc.Size &&
			Pooled.Desc.Stride == Desc.Stride &&
			Pooled.Desc.Usage == Desc.Usage;
	}

	Elaine::RHITexture* RenderGraphResourcePool::CreateRHITexture(const RGTextureDesc& Desc,
		Elaine::RHICommandContext* Context)
	{
		if (!Context)
			return nullptr;

		return Context->RHICreateTexture2D(
			Desc.Width,
			Desc.Height,
			static_cast<uint8>(Desc.Format),
			Desc.MipLevels,
			Desc.NumSamples,
			Desc.Flags,
			Desc.InitialState,
			nullptr
		);
	}

	Elaine::RHIBuffer* RenderGraphResourcePool::CreateRHIBuffer(const RGBufferDesc& Desc,
		Elaine::RHICommandContext* Context)
	{
		if (!Context)
			return nullptr;

		return Context->RHICreateBuffer(
			Desc.Size,
			Desc.Usage,
			Desc.Stride,
			Desc.InitialState,
			nullptr
		);
	}

	void RenderGraphResourcePool::DestroyRHITexture(Elaine::RHITexture* Texture)
	{
		// RHI资源通过引用计数自动管理
		// 这里只需要解除引用
		if (Texture)
		{
			// TODO: 
		}
	}

	void RenderGraphResourcePool::DestroyRHIBuffer(Elaine::RHIBuffer* Buffer)
	{
		if (Buffer)
		{
			// TODO: 
		}
	}
}
