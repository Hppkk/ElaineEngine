#include "ElainePrecompiledHeader.h"
#include "ElaineRenderQueue.h"
#include "ElainePass.h"
#include "RenderProxy/ElaineRenderProxy.h"

namespace Elaine
{
	RenderQueue::RenderQueue(NamedRenderQueue InName)
		: mName(InName)
	{

	}

	void RenderQueue::RecordRenderCommand(RHICommandList* InRHICommandCtx)
	{
		for (auto&& CurrRenderableObjects : mRenderableObjects)
		{
			for (auto&& CurrRenderableObject : CurrRenderableObjects.second)
			{
				//CurrRenderableObject->RecordRenderCommand(InRHICommandCtx);
			}
		}
	}

	void RenderQueue::UpdateRenderQueue(ShaderPass* InPass, RenderProxy* InObject, RenderQueuePriority InPriority)
	{
		if (InPass && InObject)
			UpdateRenderQueue(InPass->GetPipelineRHI(), InObject->GetResourceBinding(), InPriority);
	}

	void RenderQueue::UpdateRenderQueue(RHIPipeline* InPipeline, const RHI_DRAW_RESOURCE_BINDING& InBinding, RenderQueuePriority InPriority)
	{
		if (!InPipeline)
			return;

		auto Iter = mRenderableObjects.find(InPriority);
		if (Iter == mRenderableObjects.end())
		{
			Iter = mRenderableObjects.emplace(InPriority, std::vector<RenderablePass>()).first;
		}
		RenderablePass Element;
		Element.mPipeline = InPipeline;
		Element.mResourceBinding = InBinding;
		Iter->second.push_back(std::move(Element));
	}

	void RenderQueue::Render(RHICommandList* InRHICommandList)
	{
		for (auto&& CurrRenderableObjects : mRenderableObjects)
		{
			for (auto&& CurrElement : CurrRenderableObjects.second)
			{
				RHI_DRAW_RESOURCE_BINDING& RenderGfxState = CurrElement.mResourceBinding;
				InRHICommandList->BindGfxPipeline(CurrElement.mPipeline.get());
				InRHICommandList->BindDrawData(&RenderGfxState);
				if (RenderGfxState.mIndexBuffer)
				{
					InRHICommandList->DrawIndexedPrimitive(RenderGfxState.mIndexBuffer.get(), RenderGfxState.mVertexOffset,
						RenderGfxState.mFirstInstance, RenderGfxState.mVertexCount, RenderGfxState.mFirstIndex,
						RenderGfxState.mIndexCount / 3, RenderGfxState.mInstanceCount);
				}
				else
				{
					InRHICommandList->DrawPrimitive(RenderGfxState.mFirstVertex, RenderGfxState.mVertexCount, RenderGfxState.mInstanceCount);
				}
			}
		}
	}

	void RenderQueue::Clear()
	{
		mRenderableObjects.clear();
	}

	bool RenderQueue::IsEmpty()
	{
		return mRenderableObjects.empty();
	}

	RenderQueueSet::RenderQueueSet()
	{
		mRenderQueues[RenderQueue_Normal] = new RenderQueue(RenderQueue_Normal);
		mRenderQueues[RenderQueue_Shadow] = new RenderQueue(RenderQueue_Shadow);
		mRenderQueues[RenderQueue_Sky] = new RenderQueue(RenderQueue_Sky);
		mRenderQueues[RenderQueue_Transparent] = new RenderQueue(RenderQueue_Transparent);
		mRenderQueues[RenderQueue_Screen] = new RenderQueue(RenderQueue_Screen);
		mRenderQueues[RenderQueue_UI] = new RenderQueue(RenderQueue_UI);
	}

	RenderQueueSet::~RenderQueueSet()
	{
		for (size_t Index = RenderQueue_Normal; Index < RenderQueue_Count; ++Index)
		{
			SAFE_DELETE(mRenderQueues[Index]);
		}
	}

	RenderQueue* RenderQueueSet::GetRenderQueue(NamedRenderQueue InName)
	{
		return mRenderQueues[InName];
	}

	void RenderQueueSet::RecordRenderCommand(RenderQueue* InRenderQueue, RHICommandList* InRHICommandList)
	{
		InRenderQueue->RecordRenderCommand(InRHICommandList);
	}

	void RenderQueueSet::UpdateRenderQueue(RenderQueue* InRenderQueue, RenderProxy* InObject, RenderQueuePriority InPriority)
	{
		//InRenderQueue->UpdateRenderQueue(InObject, InPriority);
	}

	void RenderQueueSet::ClearRenderQueue()
	{
		for (size_t Index = RenderQueue_Normal; Index < RenderQueue_Count; ++Index)
		{
			mRenderQueues[Index]->Clear();
		}
	}

	bool RenderQueueSet::IsEmpty()
	{
		for (size_t Index = RenderQueue_Normal; Index < RenderQueue_Count; ++Index)
		{
			if (!mRenderQueues[Index]->IsEmpty())
			{
				return false;
			}
		}
		return true;
	}
}
