#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class RenderableObject;
	class RHICommandContext;
	class Pass;

	enum NamedRenderQueue
	{
		RenderQueue_Normal,
		RenderQueue_Sky,
		RenderQueue_Transparent,
		RenderQueue_Screen,
		RenderQueue_UI,
		RenderQueue_Count,
	};

	using RenderQueuePriority = signed long long;

	struct RenderablePass
	{
		RenderableObject* mRenderObject = nullptr;
		Pass* mRenderPass = nullptr;
	};

	class ElaineCoreExport RenderQueue
	{
	public:
		RenderQueue(NamedRenderQueue InName);
		void RecordRenderCommand(RHICommandList* InRHICommandList);
		void UpdateRenderQueue(Pass* InPass, RenderableObject* InObject, RenderQueuePriority InPriority);
		void Render(RHICommandList* InRHICommandList);
		void Clear();
	protected:
		NamedRenderQueue mName;
		std::map<RenderQueuePriority, std::vector<RenderablePass>> mRenderableObjects;
		friend class RenderQueueSet;
	};

	class ElaineCoreExport RenderQueueSet
	{
	public:
		RenderQueueSet();
		~RenderQueueSet();
		RenderQueue* GetRenderQueue(NamedRenderQueue InName);
		void RecordRenderCommand(RenderQueue* InRenderQueue, RHICommandList* InRHICommandList);
		void UpdateRenderQueue(RenderQueue* InRenderQueue, RenderableObject* InObject, RenderQueuePriority InPriority);
		void ClearRenderQueue();
	private:
		std::array<RenderQueue*, RenderQueue_Count> mRenderQueues;
	};
}