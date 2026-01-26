#pragma once
#include "ElaineSingleton.h"
#include "ElaineEditorManager.h"

namespace Editor
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