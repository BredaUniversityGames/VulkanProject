#pragma once
#include "Defines.h"
#include <string>

namespace VulkanProject
{
	
	class Window;
	class Graphics;

	struct AppConfig
	{
		std::string name = "Window";
		uint windowWidth = 800;
		uint windowHeight = 600;
	};

	class Application
	{

	public:
		Application(VulkanProject::AppConfig& info);
	private:
		void ShutDown();

		bool m_Running = true;
		Window* m_Window = nullptr;
		Graphics* m_Graphics;
	};
}

