#pragma once

namespace Elaine
{
	class ElaineCoreExport InputSystem :public Singleton<InputSystem>
	{
	public:
		InputSystem() = default;

		void PollEvent();
	private:

	};
}