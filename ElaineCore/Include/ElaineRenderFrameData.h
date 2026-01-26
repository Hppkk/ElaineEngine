#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineRenderQueue.h"
#include "ElaineUniformGPUManager.h"

namespace Elaine
{
	struct RenderFrameData
	{
		RenderQueueSet* mRenderQueueSet = nullptr;
		CommonUniformBufferCPU mCommonUniformBuffer;

		RenderFrameData()
		{
			mRenderQueueSet = new RenderQueueSet();
		}

		// Disable copy to prevent double deletion logic issues unless handled carefully
		RenderFrameData(const RenderFrameData&) = delete;
		RenderFrameData& operator=(const RenderFrameData&) = delete;

		// Move constructor
		RenderFrameData(RenderFrameData&& Other) noexcept 
		{
			mRenderQueueSet = Other.mRenderQueueSet;
			Other.mRenderQueueSet = nullptr;
			mCommonUniformBuffer = Other.mCommonUniformBuffer;
		}

		~RenderFrameData()
		{
			if (mRenderQueueSet)
			{
				delete mRenderQueueSet;
				mRenderQueueSet = nullptr;
			}
		}
		
		void Clear()
		{
			if (mRenderQueueSet)
				mRenderQueueSet->ClearRenderQueue();
		}
	};
}