#pragma once
#include "ElaineSingleton.h"
#include "GLFW/glfw3.h"

namespace Elaine
{
	class ElaineCoreExport RenderSystem :public Singleton<RenderSystem>
	{
	public:
		RenderSystem();
		~RenderSystem();
		
	private:

	};
}