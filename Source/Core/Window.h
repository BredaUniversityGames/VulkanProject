#pragma once

#include "Defines.h"
#include <string>

struct GLFWwindow;

namespace VulkanProject
{
	class Window
	{
	public:
		Window(uint aWidth, uint aHeight, std::string aName);
		bool Update();
		void Shutdown();

		uint m_Width = 0;
		uint m_Height = 0;

		// platform specific
		GLFWwindow* m_GLTWwindow = nullptr;
		bool m_Resized = false;
	private:
		static void ResizeCallback(GLFWwindow* window, int width, int height);

	};
}

