#pragma once
#include "ElaineEditorPrerequirements.h"
#include "ElaineEngine.h"

namespace Elaine
{
	class ElaineEditor
	{
	public:
		ElaineEditor(ElaineEngine* engine);
		virtual ~ElaineEditor();
		void			initialize();
		void			close();
		void			tick();
	private:
		ElaineEngine*	m_pEngine = nullptr;
	};
}