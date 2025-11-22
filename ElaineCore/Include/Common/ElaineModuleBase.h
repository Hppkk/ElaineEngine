#pragma once

namespace Elaine
{
	class ElaineCoreExport ModuleBase
	{
	public:
		ModuleBase() = default;
		virtual ~ModuleBase();
		virtual void Initialize() = 0;
		virtual void Terminate() = 0;
	};
}