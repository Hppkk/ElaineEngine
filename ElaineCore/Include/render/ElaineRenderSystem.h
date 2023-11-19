#pragma once
#include "ElaineSingleton.h"
#include "GLFW/glfw3.h"

namespace Elaine
{
	class ElaineCoreExport RenderSystem :public Singleton<RenderSystem>
	{
	public:
		RenderSystem();
		void initilize();
		RHI* getRHI() { return m_pRHI; }
		~RenderSystem();
		
	private:
		RHI* m_pRHI = nullptr;
	};
}