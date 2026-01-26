#include "ElainePrecompiledHeader.h"
#include "RenderGraph/ElaineRenderGraph.h"
#include "RenderGraph/ElaineRenderGraphResourcePool.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHITypes.h"
#include "render/common/ElaineRHI.h"

namespace RenderGraph
{
	//=============================================================================
	// 构造/析构
	//=============================================================================
	RenderDependencyGraph::RenderDependencyGraph()
		: mBuilder(this)
	{
		mResourcePool = std::make_unique<RenderGraphResourcePool>();
	}

	RenderDependencyGraph::~RenderDependencyGraph()
	{
		ClearFrameData();
		mResourcePool.reset();
	}

	//=============================================================================
	// 帧生命周期
	//=============================================================================
	void RenderDependencyGraph::BeginFrame()
	{
		// 重置资源池中资源的使用状态，确保可复用
		mResourcePool->BeginFrame();

		ClearFrameData();
		mIsCompiled = false;
		mIsExecuting = false;
		mNextResourceIndex = 0;
	}

	void RenderDependencyGraph::Compile()
	{
		if (mIsCompiled)
			return;

		// 1. 剔除无用 Pass
		CullUnusedPasses();

		// 2. 拓扑排序
		TopologicalSort();

		// 3. 资源生命周期分析
		AnalyzeResourceLifetimes();

		// 4. 计算异步计算同步点
		ComputeAsyncSyncPoints();

		// 5. 插入资源屏障
		InsertBarriers();

		// 6. 分配瞬态资源（虚拟分配，计算大小等，实际RHI创建在执行时）
		AllocateTransientResources();

		mIsCompiled = true;
	}

	void RenderDependencyGraph::Execute(Elaine::RHICommandContext* GraphicsContext,
		Elaine::RHICommandContext* ComputeContext)
	{
		if (!mIsCompiled)
		{
			Compile();
		}

		mIsExecuting = true;

		if (!GraphicsContext)
		{
			LOG_ERROR("RenderGraph: Can not find valid Graphics Context.");
			return;
		}

		// 执行所有 Pass
		for (size_t i = 0; i < mCompiledPasses.size(); ++i)
		{
			CompiledPass& PassInfo = mCompiledPasses[i];
			if (PassInfo.IsCulled)
				continue;

			Elaine::RHICommandContext* CurrentContext = GraphicsContext;
			if (PassInfo.Queue == ERGQueueType::Compute && ComputeContext)
			{
				CurrentContext = ComputeContext;
			}
			// TODO: 处理 Copy 队列上下文

			Elaine::RHICommandList* CmdList = CurrentContext->GetRHICommandListMgr()->GetCurrentCommandList();
			if (!CmdList) continue;

			// 执行前置屏障
			ExecuteBarriers(PassInfo.BarriersBefore, CmdList);

			// 执行 Pass
			ExecutePass(PassInfo, CmdList);

			// 执行后置屏障
			ExecuteBarriers(PassInfo.BarriersAfter, CmdList);

			// 释放不再使用的资源回池
			// 这里简单遍历所有资源，检查是否是最后一次使用
			for (auto& Resource : mResources)
			{
				if (Resource->GetLifetime().LastPassIndex == PassInfo.Pass->GetIndex())
				{
					if (Resource->GetState() == ERGResourceState::Transient)
					{
						if (Resource->IsTexture())
						{
							GraphTexture* Texture = static_cast<GraphTexture*>(Resource.get());
							mResourcePool->ReleaseTexture(Texture->GetRHITexture());
						}
						else if (Resource->IsBuffer())
						{
							GraphBuffer* Buffer = static_cast<GraphBuffer*>(Resource.get());
							mResourcePool->ReleaseBuffer(Buffer->GetRHIBuffer());
						}
					}
				}
			}
		}

		// 自动插入 Present 布局转换屏障
		if (!mPresentTransitions.empty())
		{
			Elaine::RHICommandList* PresentCmdList = GraphicsContext->GetRHICommandListMgr()->GetCurrentCommandList();
			if (PresentCmdList)
			{
				for (auto Handle : mPresentTransitions)
				{
					if (IsValidHandle(Handle))
					{
						GraphResource* Res = mResources[Handle.Index].get();
						if (Res->IsTexture())
						{
							GraphTexture* Tex = static_cast<GraphTexture*>(Res);
							Elaine::RHITexture* RHITex = Tex->GetRHITexture();
							if (RHITex)
							{
								Elaine::RHIResourceBarrierDesc Barrier;
								Barrier.Texture = RHITex;
								// 使用资源的实际当前状态，而不是硬编码
								Barrier.StateBefore = RHITex->GetAccess();
								Barrier.StateAfter = Elaine::ERHIAccess::Present;
								PresentCmdList->ResourceBarrier(Barrier);
							}
						}
					}
				}
			}
			mPresentTransitions.clear();
		}

		mIsExecuting = false;
	}

	void RenderDependencyGraph::EndFrame()
	{
		// 处理待提取队列
		for (auto& [Name, Handle] : mPendingExtractions)
		{
			if (IsValidHandle(Handle))
			{
				GraphResource* Res = mResources[Handle.Index].get();
				if (Res->IsTexture())
				{
					GraphTexture* Tex = static_cast<GraphTexture*>(Res);
					ExtractedTexture Extracted;
					Extracted.Texture = Tex->GetRHITexture();
					Extracted.Desc = Tex->GetDesc();
					mExtractedTextures[Name] = Extracted;
				}
			}
		}
		mPendingExtractions.clear();

		mResourcePool->Tick();
	}

	//=============================================================================
	// Pass 管理
	//=============================================================================
	void RenderDependencyGraph::AddPass(std::unique_ptr<RGPass> Pass)
	{
		Pass->SetIndex(static_cast<uint32>(mPasses.size()));
		mPasses.push_back(std::move(Pass));
	}

	RGPass* RenderDependencyGraph::GetPass(uint32 Index) const
	{
		if (Index < mPasses.size())
		{
			return mPasses[Index].get();
		}
		return nullptr;
	}

	//=============================================================================
	// 资源管理
	//=============================================================================
	RGTextureHandle RenderDependencyGraph::CreateTexture(const std::string& Name, const RGTextureDesc& Desc)
	{
		auto Resource = std::make_unique<GraphTexture>(Name, Desc);
		Resource->SetState(ERGResourceState::Transient);
		
		uint32 Index = AllocateResourceIndex();
		RGTextureHandle Handle;
		Handle.Index = Index;
		Handle.Version = 0;
		Resource->SetHandle(Handle);

		mResourceNameMap[Name] = Handle;
		mTextures.push_back(Resource.get()); // 辅助列表
		mResources.push_back(std::move(Resource));

		return Handle;
	}

	RGBufferHandle RenderDependencyGraph::CreateBuffer(const std::string& Name, const RGBufferDesc& Desc)
	{
		auto Resource = std::make_unique<GraphBuffer>(Name, Desc);
		Resource->SetState(ERGResourceState::Transient);

		uint32 Index = AllocateResourceIndex();
		RGBufferHandle Handle;
		Handle.Index = Index;
		Handle.Version = 0;
		Resource->SetHandle(Handle);

		mResourceNameMap[Name] = Handle;
		mBuffers.push_back(Resource.get());
		mResources.push_back(std::move(Resource));

		return Handle;
	}

	RGTextureHandle RenderDependencyGraph::CreatePersistentTexture(const std::string& Name, const RGTextureDesc& Desc)
	{
		auto Resource = std::make_unique<GraphTexture>(Name, Desc);
		// Persistent 资源标记为 External 状态，因为它们由资源池持久管理
		Resource->SetState(ERGResourceState::External);
		Resource->SetNeverCull(true);

		uint32 Index = AllocateResourceIndex();
		RGTextureHandle Handle;
		Handle.Index = Index;
		Handle.Version = 0;
		Resource->SetHandle(Handle);

		mResourceNameMap[Name] = Handle;
		mTextures.push_back(Resource.get());
		mResources.push_back(std::move(Resource));

		return Handle;
	}

	RGBufferHandle RenderDependencyGraph::CreatePersistentBuffer(const std::string& Name, const RGBufferDesc& Desc)
	{
		auto Resource = std::make_unique<GraphBuffer>(Name, Desc);
		Resource->SetState(ERGResourceState::External);
		Resource->SetNeverCull(true);

		uint32 Index = AllocateResourceIndex();
		RGBufferHandle Handle;
		Handle.Index = Index;
		Handle.Version = 0;
		Resource->SetHandle(Handle);

		mResourceNameMap[Name] = Handle;
		mBuffers.push_back(Resource.get());
		mResources.push_back(std::move(Resource));

		return Handle;
	}

	RGTextureHandle RenderDependencyGraph::ImportTexture(const std::string& Name, 
		Elaine::RHITexture* Texture, const RGTextureDesc& Desc)
	{
		auto Resource = std::make_unique<GraphTexture>(Name, Desc);
		Resource->SetState(ERGResourceState::External);
		Resource->SetRHITexture(Texture);
		Resource->SetNeverCull(true); // 外部资源通常不应被剔除

		uint32 Index = AllocateResourceIndex();
		RGTextureHandle Handle;
		Handle.Index = Index;
		Handle.Version = 0;
		Resource->SetHandle(Handle);

		mResourceNameMap[Name] = Handle;
		mTextures.push_back(Resource.get());
		mResources.push_back(std::move(Resource));

		return Handle;
	}

	RGBufferHandle RenderDependencyGraph::ImportBuffer(const std::string& Name, 
		Elaine::RHIBuffer* Buffer, const RGBufferDesc& Desc)
	{
		auto Resource = std::make_unique<GraphBuffer>(Name, Desc);
		Resource->SetState(ERGResourceState::External);
		Resource->SetRHIBuffer(Buffer);
		Resource->SetNeverCull(true);

		uint32 Index = AllocateResourceIndex();
		RGBufferHandle Handle;
		Handle.Index = Index;
		Handle.Version = 0;
		Resource->SetHandle(Handle);

		mResourceNameMap[Name] = Handle;
		mBuffers.push_back(Resource.get());
		mResources.push_back(std::move(Resource));

		return Handle;
	}

	RGTextureHandle RenderDependencyGraph::CreateTextureAlias(const std::string& Name, const RGResourceAlias& Alias)
	{
		// 查找源资源
		// 注意：这里的实现简化了，实际需要查找源资源的描述符来创建包含 Alias 信息的 GraphTexture
		if (!IsValidHandle(Alias.SourceHandle) || Alias.SourceHandle.Index >= mResources.size())
			return {};

		GraphResource* SourceRes = mResources[Alias.SourceHandle.Index].get();
		if (!SourceRes->IsTexture())
			return {};

		GraphTexture* SourceTexture = static_cast<GraphTexture*>(SourceRes);
		RGTextureDesc Desc = SourceTexture->GetDesc();
		// 修改格式为 Alias 格式
		if (Alias.ViewFormat != Elaine::PF_Unknown)
		{
			Desc.Format = Alias.ViewFormat;
		}

		auto Resource = std::make_unique<GraphTexture>(Name, Desc);
		Resource->SetState(ERGResourceState::Transient);
		Resource->SetAlias(Alias);

		uint32 Index = AllocateResourceIndex();
		RGTextureHandle Handle;
		Handle.Index = Index;
		Handle.Version = 0;
		Resource->SetHandle(Handle);

		mResourceNameMap[Name] = Handle;
		mTextures.push_back(Resource.get());
		mResources.push_back(std::move(Resource));

		// 增加源资源的引用计数，因为 Alias 依赖于它
		SourceRes->AddRef();

		return Handle;
	}

	void RenderDependencyGraph::UpdateResourceLifetime(RGResourceHandle Handle, uint32 PassIndex, ERGResourceAccess Access)
	{
		if (!IsValidHandle(Handle))
			return;

		GraphResource* Resource = mResources[Handle.Index].get();
		RGResourceLifetime& Lifetime = Resource->GetLifetimeMutable();

		if (Lifetime.FirstPassIndex == UINT32_MAX)
		{
			Lifetime.FirstPassIndex = PassIndex;
		}
		Lifetime.LastPassIndex = std::max(Lifetime.LastPassIndex, PassIndex);
		Lifetime.AccessFlags = Lifetime.AccessFlags | Access;
	}

	//=============================================================================
	// Extracted 资源
	//=============================================================================
	void RenderDependencyGraph::QueueTextureExtraction(const std::string& Name, RGTextureHandle Handle)
	{
		if (Handle.IsValid())
		{
			mPendingExtractions.push_back({ Name, Handle });
		}
	}

	RGTextureHandle RenderDependencyGraph::ImportExtractedTexture(const std::string& Name)
	{
		auto It = mExtractedTextures.find(Name);
		if (It != mExtractedTextures.end())
		{
			return ImportTexture(Name, It->second.Texture, It->second.Desc);
		}
		return {};  // 无效 Handle
	}

	bool RenderDependencyGraph::HasExtractedTexture(const std::string& Name) const
	{
		return mExtractedTextures.find(Name) != mExtractedTextures.end();
	}
	//=============================================================================
	// Present 布局转换
	//=============================================================================
	void RenderDependencyGraph::MarkForPresent(RGTextureHandle Handle)
	{
		if (Handle.IsValid())
		{
			mPresentTransitions.push_back(Handle);
		}
	}


	//=============================================================================
	// 查询
	//=============================================================================
	const RGTextureDesc* RenderDependencyGraph::GetTextureDesc(RGTextureHandle Handle) const
	{
		if (IsValidHandle(Handle))
		{
			GraphResource* Res = mResources[Handle.Index].get();
			if (Res->IsTexture())
			{
				return &static_cast<GraphTexture*>(Res)->GetDesc();
			}
		}
		return nullptr;
	}

	const RGBufferDesc* RenderDependencyGraph::GetBufferDesc(RGBufferHandle Handle) const
	{
		if (IsValidHandle(Handle))
		{
			GraphResource* Res = mResources[Handle.Index].get();
			if (Res->IsBuffer())
			{
				return &static_cast<GraphBuffer*>(Res)->GetDesc();
			}
		}
		return nullptr;
	}

	bool RenderDependencyGraph::IsValidHandle(RGResourceHandle Handle) const
	{
		return Handle.IsValid() && Handle.Index < mResources.size();
	}

	Elaine::RHITexture* RenderDependencyGraph::GetRHITexture(RGTextureHandle Handle) const
	{
		if (IsValidHandle(Handle))
		{
			GraphResource* Res = mResources[Handle.Index].get();
			if (Res->IsTexture())
			{
				return static_cast<GraphTexture*>(Res)->GetRHITexture();
			}
		}
		return nullptr;
	}

	Elaine::RHIBuffer* RenderDependencyGraph::GetRHIBuffer(RGBufferHandle Handle) const
	{
		if (IsValidHandle(Handle))
		{
			GraphResource* Res = mResources[Handle.Index].get();
			if (Res->IsBuffer())
			{
				return static_cast<GraphBuffer*>(Res)->GetRHIBuffer();
			}
		}
		return nullptr;
	}

	//=============================================================================
	// 内部实现
	//=============================================================================
	uint32 RenderDependencyGraph::AllocateResourceIndex()
	{
		return mNextResourceIndex++;
	}

	void RenderDependencyGraph::ClearFrameData()
	{
		mPasses.clear();
		mCompiledPasses.clear();
		mSortedPasses.clear();
		mResources.clear();
		mTextures.clear();
		mBuffers.clear();
		mResourceNameMap.clear();
		mAsyncSyncPoints.clear();
	}

	void RenderDependencyGraph::CullUnusedPasses()
	{
		// 1. 重置所有 Pass 的引用计数
		// 已经由 RGPass 构造函数完成初始化

		// 2. 将所有资源的引用者（Pass）建立反向索引
		// 这里简化算法：从后往前遍历 Pass，标记有副作用或输出被引用的 Pass
		
		// 暂时简单实现：只根据 HaveFlag(NeverCull) 和输出资源的引用情况
		// 真正的实现需要构建详细的依赖图
		
		std::vector<bool> IsPassActive(mPasses.size(), false);
		std::stack<RGPass*> ActiveStack;

		// 查找初始活跃 Pass（有副作用或输出外部资源的 Pass）
		for (auto& Pass : mPasses)
		{
			bool IsActive = !Pass->CanBeCulled();
			
			// 检查是否输出了外部资源
			if (!IsActive)
			{
				for (const auto& Output : Pass->GetTextureOutputs())
				{
					if (IsValidHandle(Output.Handle))
					{
						GraphResource* Res = mResources[Output.Handle.Index].get();
						if (Res->GetState() == ERGResourceState::External)
						{
							IsActive = true;
							break;
						}
					}
				}
			}

			if (IsActive)
			{
				IsPassActive[Pass->GetIndex()] = true;
				ActiveStack.push(Pass.get());
			}
		}

		// 迭代标记依赖
		while (!ActiveStack.empty())
		{
			RGPass* Pass = ActiveStack.top();
			ActiveStack.pop();

			// 检查输入资源，找到产生这些输入的 Pass
			auto CheckInputs = [&](const std::vector<RGResourceUsage>& Inputs) {
				for (const auto& Input : Inputs)
				{
					if (!IsValidHandle(Input.Handle)) continue;

					// 找到写入此资源的 Pass
					// 这需要遍历所有 Pass 寻找 Output
					// 优化：在构建时应该建立 Producer-Consumer 关系
					for (auto& Producer : mPasses)
					{
						if (IsPassActive[Producer->GetIndex()]) continue;

						bool IsProducer = false;
						for (const auto& Output : Producer->GetTextureOutputs())
						{
							// 这里简化了，实际需要处理版本号
							if (Output.Handle.Index == Input.Handle.Index) 
							{
								IsProducer = true;
								break;
							}
						}
						// 检查 Buffer ...

						if (IsProducer)
						{
							IsPassActive[Producer->GetIndex()] = true;
							ActiveStack.push(Producer.get());
						}
					}
				}
			};

			CheckInputs(Pass->GetTextureInputs());
			CheckInputs(Pass->GetBufferInputs());
		}

		// 标记被剔除的 Pass
		mStatistics.CulledPasses = 0;
		for (size_t i = 0; i < mPasses.size(); ++i)
		{
			if (!IsPassActive[i])
			{
				mStatistics.CulledPasses++;
			}
		}
	}

	void RenderDependencyGraph::TopologicalSort()
	{
		// 简单实现：保持添加顺序（假设用户已经按正确顺序添加，或者后续实现真正的排序）
		// 在完全实现依赖分析前，保持顺序是最安全的
		mSortedPasses.clear();
		for (auto& Pass : mPasses)
		{
			// if (!IsCulled(Pass)) ...
			mSortedPasses.push_back(Pass.get());
		}

		// 构建 CompiledPass 列表
		mCompiledPasses.resize(mPasses.size());
		for (size_t i = 0; i < mPasses.size(); ++i)
		{
			mCompiledPasses[i].Pass = mPasses[i].get();
			mCompiledPasses[i].Queue = mPasses[i]->GetQueueType();
			// IsCulled 标记...
		}
	}

	void RenderDependencyGraph::AnalyzeResourceLifetimes()
	{
		// 已经由 Builder 在 UpdateResourceLifetime 中部分完成
		// 这里可以进行更全局的分析
	}

	void RenderDependencyGraph::ComputeAsyncSyncPoints()
	{
		// TODO: 分析队列切换，插入信号量
	}

	void RenderDependencyGraph::InsertBarriers()
	{
		// 跟踪每个资源的当前状态
		struct ResourceStateTracker
		{
			ERHIAccess CurrentState = ERHIAccess::Unknown;
			ERGQueueType CurrentQueue = ERGQueueType::Graphics;
		};
		std::vector<ResourceStateTracker> Trackers(mResources.size());

		// 初始化外部资源状态
		for (size_t i = 0; i < mResources.size(); ++i)
		{
			if (mResources[i]->GetState() == ERGResourceState::External)
			{
				if (mResources[i]->IsTexture())
				{
					Trackers[i].CurrentState = static_cast<GraphTexture*>(mResources[i].get())->GetDesc().InitialState;
					RHITexture* Texture = static_cast<RHITexture*>(mResources[i]->GetRHIHandle());
					if (Texture && Texture->GetAccess() != ERHIAccess::Unknown)
					{
						Trackers[i].CurrentState = Texture->GetAccess();
					}
				}
				else if (mResources[i]->IsBuffer())
				{
					Trackers[i].CurrentState = static_cast<GraphBuffer*>(mResources[i].get())->GetDesc().InitialState;
				}
			}
		}

		// 遍历编译后的 Pass 插入屏障
		for (auto& Compiled : mCompiledPasses)
		{
			if (Compiled.IsCulled) continue;

			RGPass* Pass = Compiled.Pass;

			// 检查所有输入资源
			auto ProcessResource = [&](RGResourceHandle Handle, Elaine::ERHIAccess TargetState) {
				if (!IsValidHandle(Handle)) return;

				ResourceStateTracker& Tracker = Trackers[Handle.Index];
				
				// 如果状态不匹配或队列不同，插入屏障
				if (Tracker.CurrentState != TargetState /* || Queue Mismatch */)
				{
					RGBarrier Barrier;
					Barrier.Resource = Handle;
					Barrier.StateBefore = Tracker.CurrentState;
					Barrier.StateAfter = TargetState;
					// Barrier.QueueBefore = Tracker.CurrentQueue;
					// Barrier.QueueAfter = Compiled.Queue;

					Compiled.BarriersBefore.push_back(Barrier);

					// 更新追踪状态
					Tracker.CurrentState = TargetState;
				}
			};

			// 处理纹理输入
			for (const auto& Input : Pass->GetTextureInputs())
			{
				ProcessResource(Input.Handle, Input.RHIState);
			}

			// 处理渲染目标（作为输出，但也需要转换到 RTV 状态）
			for (const auto& RT : Pass->GetRenderTargets())
			{
				// 通常 RenderPass 开始时会自动转换，但如果没有自动转换支持，需要显式屏障
				// 这里假设需要显式屏障转换到 RTV
				ProcessResource(RT.Handle, Elaine::ERHIAccess::RTV);
			}

			if (Pass->GetDepthStencil())
			{
				auto& DS = *Pass->GetDepthStencil();
				bool ReadOnly = DS.Desc.ReadOnly;
				ProcessResource(DS.Handle, ReadOnly ? Elaine::ERHIAccess::DSVRead : Elaine::ERHIAccess::DSVWrite);
			}

			// 处理输出
			for (const auto& Output : Pass->GetTextureOutputs())
			{
				// 输出资源在 Pass 执行后状态确定
				// 但如果在 Pass 执行前需要处于特定状态（例如 RTV），已在上面处理
				// 这里更新追踪器状态为 Pass 结束后的状态
				Trackers[Output.Handle.Index].CurrentState = Output.RHIState;
			}
			
			// 处理 Buffer...
		}
	}

	void RenderDependencyGraph::AllocateTransientResources()
	{
		// 实际分配推迟到 Execute 阶段，利用 ResourcePool
	}

	void RenderDependencyGraph::ExecutePass(CompiledPass& PassInfo, Elaine::RHICommandList* CmdList)
	{
		RGPass* Pass = PassInfo.Pass;

		// 1. 实现资源分配
		// 遍历 Pass 使用的所有资源，如果是首次使用，从池中获取
		auto AllocateIfNeeded = [&](RGResourceHandle Handle, const std::string& ResourceName = "") {
			if (!IsValidHandle(Handle)) return;
			GraphResource* Res = mResources[Handle.Index].get();
			
			if (Res->GetState() == ERGResourceState::Transient)
			{
				// 瞬态资源：从池中获取
				if (Res->IsTexture())
				{
					GraphTexture* Tex = static_cast<GraphTexture*>(Res);
					if (Tex->GetRHITexture() == nullptr)
					{
						// 如果有 Alias，尝试复用源资源
						if (Tex->HasAlias())
						{
							RGResourceHandle SourceHandle = Tex->GetAlias().SourceHandle;
							if (IsValidHandle(SourceHandle))
							{
								GraphResource* SourceRes = mResources[SourceHandle.Index].get();
								if (SourceRes->IsTexture())
								{
									// 这是一个简化，实际 RHI 需要支持创建 View
									// 这里假设我们只是重新解释
									Tex->SetRHITexture(static_cast<GraphTexture*>(SourceRes)->GetRHITexture());
								}
							}
						}
						else
						{
							Tex->SetRHITexture(mResourcePool->AcquireTexture(Tex->GetDesc(), CmdList->GetCmdContext()));
						}
					}
				}
				else if (Res->IsBuffer())
				{
					GraphBuffer* Buf = static_cast<GraphBuffer*>(Res);
					if (Buf->GetRHIBuffer() == nullptr)
					{
						Buf->SetRHIBuffer(mResourcePool->AcquireBuffer(Buf->GetDesc(), CmdList->GetCmdContext()));
					}
				}
			}
			else if (Res->GetState() == ERGResourceState::External && Res->GetRHIHandle() == nullptr)
			{
				// External 状态但无 RHI 资源：可能是 Persistent 资源，从持久池获取
				if (Res->IsTexture())
				{
					GraphTexture* Tex = static_cast<GraphTexture*>(Res);
					std::string Name = ResourceName.empty() ? Res->GetName() : ResourceName;
					Tex->SetRHITexture(mResourcePool->AcquirePersistentTexture(Name, Tex->GetDesc(), CmdList->GetCmdContext()));
				}
				else if (Res->IsBuffer())
				{
					GraphBuffer* Buf = static_cast<GraphBuffer*>(Res);
					std::string Name = ResourceName.empty() ? Res->GetName() : ResourceName;
					Buf->SetRHIBuffer(mResourcePool->AcquirePersistentBuffer(Name, Buf->GetDesc(), CmdList->GetCmdContext()));
				}
			}
		};

		// 检查所有输入输出
		for (const auto& In : Pass->GetTextureInputs()) AllocateIfNeeded(In.Handle);
		for (const auto& Out : Pass->GetTextureOutputs()) AllocateIfNeeded(Out.Handle);
		for (const auto& In : Pass->GetBufferInputs()) AllocateIfNeeded(In.Handle);
		for (const auto& Out : Pass->GetBufferOutputs()) AllocateIfNeeded(Out.Handle);
		for (const auto& RT : Pass->GetRenderTargets()) AllocateIfNeeded(RT.Handle);
		if (Pass->GetDepthStencil()) AllocateIfNeeded(Pass->GetDepthStencil()->Handle);

		// 2. 对于 Raster Pass，开始 RenderPass
		bool bIsRasterPass = (Pass->GetType() == PassType::Raster);
		if (bIsRasterPass)
		{
			// 构建 RenderPassInfo
			RHIRenderPassInfo RenderPassInfo;
			
			// 设置颜色目标
			const auto& RenderTargets = Pass->GetRenderTargets();
			RenderPassInfo.NumColorRenderTargets = static_cast<uint32>(RenderTargets.size());
			for (size_t i = 0; i < RenderTargets.size() && i < 8; ++i)
			{
				const auto& RT = RenderTargets[i];
				if (IsValidHandle(RT.Handle))
				{
					GraphTexture* Tex = static_cast<GraphTexture*>(mResources[RT.Handle.Index].get());
					RenderPassInfo.ColorRenderTargets[i].RenderTarget = Tex->GetRHITexture();
					RenderPassInfo.ColorRenderTargets[i].Action = RT.Desc.LoadStoreOp;
					RenderPassInfo.ColorRenderTargets[i].ClearColor = RT.Desc.ClearColor;
				}
			}

			// 设置深度目标
			if (Pass->GetDepthStencil())
			{
				const auto& DS = *Pass->GetDepthStencil();
				if (IsValidHandle(DS.Handle))
				{
					GraphTexture* Tex = static_cast<GraphTexture*>(mResources[DS.Handle.Index].get());
					RenderPassInfo.DepthStencilRenderTarget.DepthStencilTarget = Tex->GetRHITexture();
					RenderPassInfo.DepthStencilRenderTarget.Action = DS.Desc.LoadStoreOp;
					RenderPassInfo.DepthStencilRenderTarget.ClearDepth = DS.Desc.ClearDepth;
					RenderPassInfo.DepthStencilRenderTarget.ClearStencil = DS.Desc.ClearStencil;
				}
			}

			// 开始 RenderPass
			CmdList->BeginRenderPassInfo(RenderPassInfo, Pass->GetName().c_str());
		}

		// 3. 执行回调
		Pass->Execute(CmdList);

		// 4. 对于 Raster Pass，结束 RenderPass
		if (bIsRasterPass)
		{
			CmdList->EndRenderPass();
		}
	}

	void RenderDependencyGraph::ExecuteBarriers(const std::vector<RGBarrier>& Barriers,
		Elaine::RHICommandList* CmdList)
	{
		if (!CmdList) return;

		for (const auto& Barrier : Barriers)
		{
			if (IsValidHandle(Barrier.Resource))
			{
				GraphResource* Res = mResources[Barrier.Resource.Index].get();
				
				Elaine::RHIResourceBarrierDesc BarrierDesc;
				BarrierDesc.StateBefore = Barrier.StateBefore;
				BarrierDesc.StateAfter = Barrier.StateAfter;
				
				if (Res->IsTexture())
				{
					GraphTexture* TexRes = static_cast<GraphTexture*>(Res);
					BarrierDesc.Texture = TexRes->GetRHITexture();
					BarrierDesc.Buffer = nullptr;
				}
				else if (Res->IsBuffer())
				{
					GraphBuffer* BufRes = static_cast<GraphBuffer*>(Res);
					BarrierDesc.Buffer = BufRes->GetRHIBuffer();
					BarrierDesc.Texture = nullptr;
				}
				else
				{
					continue;  // 未知资源类型
				}
				
				CmdList->ResourceBarrier(BarrierDesc);
			}
		}
	}

	void RenderDependencyGraph::DumpGraph(const std::string& FilePath) const
	{
		std::ofstream File(FilePath);
		if (!File.is_open()) return;

		File << "digraph RenderGraph {\n";
		File << "  rankdir=LR;\n";

		// Nodes: Passes
		for (const auto& Pass : mPasses)
		{
			File << "  Pass" << Pass->GetIndex() << " [label=\"" << Pass->GetName() << "\", shape=box";
			if (Pass->GetType() == PassType::AsyncCompute) File << ", style=filled, fillcolor=lightblue";
			File << "];\n";
		}

		// Nodes: Resources
		for (const auto& Res : mResources)
		{
			File << "  Res" << Res->GetHandle().Index << " [label=\"" << Res->GetName() << "\", shape=ellipse];\n";
		}

		// Edges
		for (const auto& Pass : mPasses)
		{
			for (const auto& Input : Pass->GetTextureInputs())
			{
				if(IsValidHandle(Input.Handle))
					File << "  Res" << Input.Handle.Index << " -> Pass" << Pass->GetIndex() << ";\n";
			}
			for (const auto& Output : Pass->GetTextureOutputs())
			{
				if (IsValidHandle(Output.Handle))
				File << "  Pass" << Pass->GetIndex() << " -> Res" << Output.Handle.Index << ";\n";
			}
		}

		File << "}\n";
	}
}
