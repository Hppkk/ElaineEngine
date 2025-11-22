#pragma once
#include "ElaineSingleton.h"

namespace ElaineEditor
{
	class EditorGlobalContext : public Elaine::Singleton<EditorGlobalContext>
	{
	public:
		EditorGlobalContext();
		~EditorGlobalContext();
		void Initialize();
	private:

	};
}