#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"

namespace Elaine
{
	enum EngineMode
	{
		em_Editor,
		em_Runtime,
	};

	class ElaineCoreExport Root :public Singleton<Root>
	{
	public:
		Root() = default;
		Root(EngineMode mode);
		~Root();
		void					Init();
		EngineMode				GetEngineMode()const { return m_currentMode; }
	private:
		// жуж╧
		void					terminate();
	private:
		EngineMode				m_currentMode = em_Editor;
	};

}