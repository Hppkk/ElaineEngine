#pragma once
#include "common/ElaineEnginePrerequirements.h"
#include "ElaineRoot.h"
#include <string>


namespace Elaine
{
	struct EngineInitDesc
	{

		std::string			m_sConfigFile;
	};

	class ElaineEngineExport ElaineEngine
	{
	public:
		ElaineEngine() = default;
		virtual ~ElaineEngine();

		void		Initilize(const RHI_PARAM_DESC& InRendererDesc);
		void		RenderOneFrame();
		void		RenderOneFrame(float InDeltaTime);
		void		DestroyEngine();
	};
}