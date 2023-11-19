#include "ElainePrecompiledHeader.h"
#include "ElaineInputSystem.h"

namespace Elaine
{
	void InputSystem::PollEvent()
	{
		glfwPollEvents();
	}
}