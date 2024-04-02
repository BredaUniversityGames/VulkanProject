#include "Window.h"
#include "includes.h"

// For expections for now
#include <iostream>

VulkanProject::Window::Window(uint aWidth, uint aHeight, std::string aName) : m_Width(aWidth), m_Height(aHeight)
{
	//Initialise window 
	glfwInit();

	// Not using OpenGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// Not yet implemented
	m_GLTWwindow = glfwCreateWindow(m_Width, m_Height, aName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(m_GLTWwindow, this);
	glfwSetFramebufferSizeCallback(m_GLTWwindow, ResizeCallback);

	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

}

bool VulkanProject::Window::Update()
{
	
	if (!glfwWindowShouldClose(m_GLTWwindow))
	{
		glfwPollEvents();
		return true;
	}
	else return false;
	
}

void VulkanProject::Window::Shutdown()
{
	//Destroy window

	glfwDestroyWindow(m_GLTWwindow);

	glfwTerminate();
}

 void VulkanProject::Window::ResizeCallback(GLFWwindow* window, int width, int height)
{
	auto Awindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	Awindow->m_Resized = true;
	Awindow->m_Width = width;
	Awindow->m_Height = height;
}
