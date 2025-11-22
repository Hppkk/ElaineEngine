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
		ElaineEngine();
		virtual ~ElaineEngine();

		void		Initialize(const RHI_PARAM_DESC& InRendererDesc);
		void		RenderOneFrame();
		void		RenderOneFrame(float InDeltaTime);
		void		DestroyEngine();
	};

	extern ElaineEngineExport ElaineEngine* GetElaineEngine();
}