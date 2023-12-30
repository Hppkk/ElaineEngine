#pragma once
#include "ElaineSingleton.h"
#include "common/ElaineEnginePrerequirements.h"

namespace Elaine
{
	class ElaineEngineExport ComponentSystem :public Singleton<ComponentSystem>
	{
	public:
		ComponentSystem();
		~ComponentSystem();
		void						registerComFactory();
	};
}