#pragma once
#include "ElaineSingleton.h"
#include "GLFW/glfw3.h"
#include "math/ElaineVector2.h"

namespace Elaine
{
	class ElaineCoreExport WindowSystem :public Singleton<WindowSystem>
	{
	public:
		WindowSystem();
		~WindowSystem();
		GLFWwindow* createWindow(int width, int height, const String& name);
		GLFWwindow* createMainWindow(int width, int height, const String& name);

		GLFWwindow* getWindow(const String& name);
		Vector2 getWindowSize(const String& name);
		void tick();

		void destoryWindow(const String& name);
		void destoryWindow(GLFWwindow* window);
		void Terminate();

	private:
		std::map<String, GLFWwindow*>  m_windows;
		std::map<GLFWwindow*, Vector2> m_windowSize;
		GLFWwindow* m_mainWindow = nullptr;
	};
}