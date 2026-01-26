#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"

namespace Elaine
{
	class ElaineCoreExport FileMonitor : public Singleton<FileMonitor>
	{
	public:
		FileMonitor() = default;
		~FileMonitor();
	};
}