#include "ElainePrecompiledHeader.h"
#include "ElaineRenderQueue.h"
#include "ElainePass.h"

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

	void RenderQueue::UpdateRenderQueue(Pass* InPass, RenderableObject* InObject, RenderQueuePriority InPriority)
	{
		auto Iter = mRenderableObjects.find(InPriority);
		if (Iter == mRenderableObjects.end())
		{
			Iter = mRenderableObjects.emplace(InPriority, std::vector<RenderablePass>()).first;
		}
		RenderablePass Element;
		Element.mRenderPass = InPass;
		Element.mRenderObject = InObject;
		Iter->second.push_back(Element);
	}

	void RenderQueue::Render(RHICommandList* InRHICommandList)
	{
		for (auto&& CurrRenderableObjects : mRenderableObjects)
		{
			for (auto&& CurrElement : CurrRenderableObjects.second)
			{
				GRAPHICS_PIPELINE_STATE_DESC& RenderGfxState = CurrElement.mRenderPass->GetGfxState();
				//InRHICommandList->BeginRenderPass(RenderGfxState);
				InRHICommandList->BindGfxPipeline(CurrElement.mRenderPass->GetRHIPipeline());
				InRHICommandList->BindDrawData(&RenderGfxState);
				InRHICommandList->DrawPrimitive(0, 1, 1);
				//InRHICommandList->EndRenderPass();
			}
		}
	}

	void RenderQueue::Clear()
	{
		mRenderableObjects.clear();
	}

	RenderQueueSet::RenderQueueSet()
	{
		mRenderQueues[RenderQueue_Normal] = new RenderQueue(RenderQueue_Normal);
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

	void RenderQueueSet::UpdateRenderQueue(RenderQueue* InRenderQueue, RenderableObject* InObject, RenderQueuePriority InPriority)
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
}
