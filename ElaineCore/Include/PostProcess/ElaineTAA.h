#pragma once
#include "ElaineCorePrerequirements.h"
#include "PostProcess/ElainePostProcessBase.h"

namespace Elaine
{
	class ElaineCoreExport TAA : public PostProcessBase
	{
	public:
		TAA();
		virtual ~TAA();
		virtual void Initialize() override;
		virtual void Render() override;
	};
}