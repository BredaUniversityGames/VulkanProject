#include "Application.h"
#include "Rendering/Graphics.h"
#include "Window.h"
#include "Rendering/Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
VulkanProject::Application::Application(VulkanProject::AppConfig& info)
{
	// Creating window
	m_Window = new Window(info.windowWidth, info.windowHeight, info.name);

	// Initialising rendering
	m_Graphics = new Graphics(m_Window);
	m_Graphics->Init(info.windowWidth, info.windowHeight, info.name);

	glm::vec4 color = { 0.5f,0.3f,0.5f, 1.f };
	Renderer::SetClearColor(color);

	PipelineDesc desc;
	desc.vertexShaderPath = "Resources/Shaders/vert.spv"; 
	desc.fragmentShaderPath = "Resources/Shaders/frag.spv"; 

	const std::vector<Vertex> vertices = 
	{
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
		
	};


	const std::vector<uint16_t> indices = 
	{
		0, 1, 2, 2, 3, 0
	};

	Mesh mesh{ vertices, indices };
	GraphicsPipeline pipeline(desc);
	pipeline.Bind();
	
	// Main loop
	while (m_Window->Update())
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), m_Window->m_Width / (float)m_Window->m_Height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		m_Graphics->BeginFrame();
		pipeline.UpdateBuffers(ubo);
		mesh.Draw();
		m_Graphics->EndFrame();
		
	}
	ShutDown();
	
}

void VulkanProject::Application::ShutDown()
{
	// Shutting down inverse order

	m_Graphics->Shutdown();
	m_Graphics = nullptr;

	m_Window->Shutdown();
	m_Window = nullptr;
}
