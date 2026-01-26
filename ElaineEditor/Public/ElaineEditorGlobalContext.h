#pragma once
#include "ElaineSingleton.h"

namespace Editor
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