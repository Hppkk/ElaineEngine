#pragma once
#include "ElaineSingleton.h"
#include "math/ElaineVector2.h"

struct GLFWwindow;

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
		GLFWwindow* getMainWindow() { return m_mainWindow; }
		Vector2 getWindowSize(const String& name);

		void destoryWindow(const String& name);
		void destoryWindow(GLFWwindow* window);
		bool shouldClose(GLFWwindow* window) const;
		void Terminate();

	private:
		std::map<String, GLFWwindow*>  m_windows;
		std::map<GLFWwindow*, Vector2> m_windowSize;
		GLFWwindow* m_mainWindow = nullptr;
	};
}