#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"

namespace Elaine
{
	class ElaineCoreExport FileSystem :public Singleton<FileSystem>
	{
	public:
		FileSystem() = default;
		~FileSystem();
	};
}