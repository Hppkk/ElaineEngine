#pragma once
#include "ElaineEditorPrerequirements.h"
#include "ElaineEditorGlobalContext.h"
#include "ElaineEngine.h"

namespace ElaineEditor
{
	class ElaineEditor
	{
	public:
		ElaineEditor(Elaine::ElaineEngine* InEngine);
		virtual ~ElaineEditor();
		void			Initilize();
		void			Destroy();
		void			Tick();
	private:
		Elaine::ElaineEngine* mEngineImpl = nullptr;
	};
}