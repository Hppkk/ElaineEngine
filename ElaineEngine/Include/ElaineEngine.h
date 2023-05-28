#pragma once
#include "common/ElaineEnginePrerequirements.h"
#include "common/ElainePrecompiledHeader.h"


namespace Elaine
{
	struct EngineInitDesc
	{

		std::string			m_sConfigFile;
	};

	class ElaineEngineExport Engine
	{
		Engine() = default;
		~Engine();

		void		initilize(const EngineInitDesc& desc);
		void		tickOneFrame(float dt);
	};
}