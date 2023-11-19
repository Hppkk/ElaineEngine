#pragma once
#include "common/ElaineEnginePrerequirements.h"
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

		void		initilize(const EngineInitDesc& desc);
		void		tickOneFrame();
		void		close();
	};
}