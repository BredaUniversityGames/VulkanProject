#pragma once
#include "Core/includes.h"
#include "Core/Defines.h"
#include <vector>



namespace VulkanProject
{
	class Window;
#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif// _DEBUG
	namespace Renderer
	{
		void SetClearColor(glm::vec4& color);
		void BindPipeline(const VkPipeline& pipeline);
		const VkRenderPass GetRenderPass();
		const VkDevice GetDevice();
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void BindBuffer(const VkBuffer* buffer, uint32_t sizeOfBuffer);
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		
	}
	class Graphics
	{
	public:
		Graphics(Window* window);
		void Init(uint& width, uint& height, std::string& name);
		void Shutdown();
		~Graphics();
		void DrawFrame();
		void Resize();
		void BeginFrame();
		void EndFrame();
	private:
	
		void CreateSwapChain();
		void ClearSwapChain();
		void CreateImageViews();
		void CreateFrameBuffers();
		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	
		// variables
		const int MAX_FRAMES_IN_FLIGHT = 2;
		//uint32_t m_CurrentFrame = 0;

		Window* m_WindowInstance = nullptr;
		
		// Vulkan specific objects
		VkInstance m_Instance = nullptr;
		VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;

		//VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

		//VkQueue m_GraphicsQueue = nullptr;
		VkQueue m_PresentQueue = nullptr;
		VkSurfaceKHR m_Surface = nullptr;

		VkSwapchainKHR m_SwapChain = nullptr;
		std::vector<VkImage>m_SwapChainImages;
		VkFormat m_SwapChainImageFormat;
		VkExtent2D m_SwapChainExtent;
		std::vector<VkImageView> m_SwapChainImageViews;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;

		//VkCommandPool m_CommandPool;
		//std::vector<VkCommandBuffer> m_CommandBuffers;

		// sync objects
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		
		uint32_t m_ImageIndex;
		VkResult m_Result;
	};
}

