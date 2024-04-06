#pragma once
#include "Core/includes.h"
#include "Core/Defines.h"
#include <vector>

static const unsigned int MAX_FRAMES_IN_FLIGHT = 2;

namespace VulkanProject
{
	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	class Window;
#ifdef _DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif// _DEBUG
	namespace Renderer
	{
		const uint GetCurrentFrame();
		template <typename T> void UploadUniformBuffer(std::vector<void*> buffer, T adata, size_t sizeOfData) 
		{
			memcpy(buffer[GetCurrentFrame()], &adata, sizeOfData);
		};

		void SetClearColor(glm::vec4& color);
		void BindPipeline(const VkPipeline& pipeline, const VkPipelineLayout layout);
		const VkRenderPass GetRenderPass();
		const VkDevice GetDevice();
		const VkPhysicalDevice GetPhysicalDevice();

		void UploadBuffer(const VkBuffer* buffer, uint32_t sizeOfBuffer);
		void UploadIndexedBuffer(const VkBuffer* buffer, VkBuffer indexBuffer, uint32_t sizeOfIndices);

		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

		void BindDescriptors(std::vector<VkDescriptorSet> descriptors);

		void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

		VkCommandBuffer BeginSingleTimeCommands();
		void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

		VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
		
	}
	class Graphics
	{
	public:
		Graphics(Window* window);
		void Init(uint& width, uint& height, std::string& name);
		void Shutdown();
		~Graphics();
		
		void Resize();
		void BeginFrame();
		void EndFrame();
	private:
		void CreateDepthResources();
		void CreateSwapChain();
		void ClearSwapChain();
		void CreateImageViews();
		void CreateFrameBuffers();
		
	
		// variables
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
		//VkExtent2D m_SwapChainExtent;
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

		VkImage m_DepthImage;
		VkDeviceMemory m_DepthImageMemory;
		VkImageView m_DepthImageView;
	};
}

