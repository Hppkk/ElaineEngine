#pragma once
#include "ElaineEnginePrerequirements.h"

namespace Elaine
{
	using LogicCommand = void(*)();

	class ElaineEngineExport LogicCommandQueue
	{
	public:
		LogicCommandQueue();
	};
}