#include "Application.h"
#include "Rendering/Graphics.h"
#include "Window.h"
#include "Rendering/Shader.h"
#include <array>
struct Vertex 
{
	glm::vec2 pos{};
	glm::vec3 color{};

	VkVertexInputBindingDescription getBindingDescription() 
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() 
	{
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};
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

	GraphicsPipeline pipeline(desc);
	pipeline.Bind();

	const std::vector<Vertex> vertices = 
	{
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	// Main loop
	while (m_Window->Update())
	{
		
		m_Graphics->DrawFrame();
		
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
