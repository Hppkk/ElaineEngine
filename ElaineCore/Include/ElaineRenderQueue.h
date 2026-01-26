#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class RenderProxy;
	class RHICommandContext;
	class ShaderPass;

	enum NamedRenderQueue
	{
		RenderQueue_Normal,
		RenderQueue_Shadow,
		RenderQueue_Sky,
		RenderQueue_Transparent,
		RenderQueue_Screen,
		RenderQueue_UI,
		RenderQueue_Count,
	};

	using RenderQueuePriority = signed long long;

	struct RenderablePass
	{
		RenderProxy* mRenderObject = nullptr;
		ShaderPass* mRenderPass = nullptr;
	};

	class ElaineCoreExport RenderQueue
	{
	public:
		RenderQueue(NamedRenderQueue InName);
		void RecordRenderCommand(RHICommandList* InRHICommandList);
		void UpdateRenderQueue(ShaderPass* InPass, RenderProxy* InObject, RenderQueuePriority InPriority);
		void Render(RHICommandList* InRHICommandList);
		void Clear();
		bool IsEmpty();
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
		void UpdateRenderQueue(RenderQueue* InRenderQueue, RenderProxy* InObject, RenderQueuePriority InPriority);
		void ClearRenderQueue();
		bool IsEmpty();
	private:
		std::array<RenderQueue*, RenderQueue_Count> mRenderQueues;
	};
}