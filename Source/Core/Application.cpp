#include "Application.h"
#include "Rendering/Graphics.h"
#include "Window.h"
#include "Rendering/Shader.h"

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
		{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	Mesh mesh{ vertices };
	GraphicsPipeline pipeline(desc);
	pipeline.Bind();


	// Main loop
	while (m_Window->Update())
	{
		m_Graphics->BeginFrame();
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
