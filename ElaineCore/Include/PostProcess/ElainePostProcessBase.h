#pragma once
#include "ElaineCorePrerequirements.h"
#include "PostProcess/ElainePostProcessUtils.h"

namespace Elaine
{
	class ElaineCoreExport PostProcessBase
	{
	public:
		PostProcessBase();
		virtual ~PostProcessBase();
		virtual void Initialize();
		virtual void Render();
		PostProcessType GetType() const { return mType; }
	protected:
		PostProcessType mType;
	};
}