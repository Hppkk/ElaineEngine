#pragma once
#include "ElaineCorePrerequirements.h"


namespace Elaine
{
	class RenderView;

	enum RenderPipelineType
	{
		RP_Forward,
		RP_Defferred,
		RP_ForwardPlus,
		RP_Count
	};

	class ElaineCoreExport RenderPipeline
	{
	public:
		RenderPipeline();
		virtual ~RenderPipeline();
		virtual void Initialize() = 0;
		virtual void Render(RenderView* InRenderView) = 0;
		RenderPipelineType GetType() const { return mRPType; }
	protected:
		RenderPipelineType mRPType;
	};
}