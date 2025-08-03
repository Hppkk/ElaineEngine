#pragma once
#include "ElaineSingleton.h"
#include "ElaineEditorManager.h"

namespace ElaineEditor
{

	class EditorSystem : public Elaine::Singleton<EditorSystem>
	{
	public:
		EditorSystem();
		~EditorSystem();
	private:
		std::map<EditorType, EditorManagerBase*> mEditorManagers;
	};
}