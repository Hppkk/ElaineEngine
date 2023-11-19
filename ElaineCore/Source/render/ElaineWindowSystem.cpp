#include "ElainePrecompiledHeader.h"
#include "render/ElaineWindowSystem.h"
#include "GLFW/glfw3.h"

namespace Elaine
{
	WindowSystem::WindowSystem()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	}

	WindowSystem::~WindowSystem()
	{
		Terminate();
		m_windows.clear();
		m_windowSize.clear();
	}


	GLFWwindow* WindowSystem::createWindow(int width, int height, const String& name)
	{
		GLFWwindow* window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
		m_windows.emplace(name, window);
		m_windowSize.emplace(window, Vector2((float)width, (float)height));
		return window;
	}

	GLFWwindow* WindowSystem::createMainWindow(int width, int height, const String& name)
	{
		if (m_mainWindow != nullptr)
			return nullptr;

		auto win = createWindow(width, height, name);
		m_mainWindow = win;
		return win;
	}

	GLFWwindow* WindowSystem::getWindow(const String& name)
	{
		auto it = m_windows.find(name);
		if (it != m_windows.end())
			return it->second;
		return nullptr;
	}

	Vector2 WindowSystem::getWindowSize(const String& name)
	{
		auto it = m_windows.find(name);
		if (it == m_windows.end())
			return {};

		return m_windowSize[it->second];
	}

	void WindowSystem::destoryWindow(const String& name)
	{
		auto iter = m_windows.find(name);
		if (iter == m_windows.end())
			return;

		glfwDestroyWindow(iter->second);
		m_windows.erase(name);
	}

	void WindowSystem::destoryWindow(GLFWwindow* window)
	{
		glfwDestroyWindow(window);
	}

	bool WindowSystem::shouldClose(GLFWwindow* window) const { return glfwWindowShouldClose(window); }

	void WindowSystem::Terminate()
	{
		for (auto&& iter : m_windows)
		{
			glfwDestroyWindow(iter.second);
		}
		glfwTerminate();
	}
}