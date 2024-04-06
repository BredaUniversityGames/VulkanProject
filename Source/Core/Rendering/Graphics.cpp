#include "Graphics.h"
#include "Core/Window.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <array>
#include "HelperFunctions.h"
 


struct RenderData
{
	VkRenderPass m_RenderPass;
	VkDevice m_Device = nullptr;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	
	VkPipeline m_BoundPipeline;
	VkPipelineLayout m_PipelineLayout;
	VkClearValue m_ClearColor = { 0.f,0.f,0.f,0.f };
	std::vector<VkCommandBuffer> m_CommandBuffers;
	uint32_t m_CurrentFrame = 0;
	VkCommandPool m_CommandPool;
	VkQueue m_GraphicsQueue = nullptr;
	VkExtent2D m_SwapChainExtent;

	std::vector<VkDeviceSize> m_Offset;
};
static RenderData* data;


VulkanProject::Graphics::Graphics(Window* window) : m_WindowInstance(window)
{
	data = new RenderData();
	
}

void VulkanProject::Graphics::CreateDepthResources()
{
	
	VkFormat depthFormat = findDepthFormat(data->m_PhysicalDevice);

	Renderer::CreateImage(data->m_SwapChainExtent.width, data->m_SwapChainExtent.height, depthFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);

	m_DepthImageView = Renderer::CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	
}
void VulkanProject::Graphics::Init(uint& width, uint& height, std::string& name)
{

	//Creating instance
	{
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = name.c_str();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
			createInfo.ppEnabledLayerNames = g_validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	// Set up validation layer
	{
		if (enableValidationLayers) 
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo;
			populateDebugMessengerCreateInfo(createInfo);

			if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) 
			{
				throw std::runtime_error("failed to set up debug messenger!");
			}
		}
	}

	// Create surface
	{
		if (glfwCreateWindowSurface(m_Instance, m_WindowInstance->m_GLTWwindow, nullptr, &m_Surface) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create window surface!");
		}
	}

	// Pick a physical device
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device, m_Surface))
			{
				data->m_PhysicalDevice = device;
				break;
			}
		}

		if (data->m_PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	// Create a logical device
	{
		QueueFamilyIndices indices = findQueueFamilies(data->m_PhysicalDevice, m_Surface);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(g_deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();

		if (enableValidationLayers) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
			createInfo.ppEnabledLayerNames = g_validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(data->m_PhysicalDevice, &createInfo, nullptr, &data->m_Device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(data->m_Device, indices.graphicsFamily.value(), 0, &data->m_GraphicsQueue);
		vkGetDeviceQueue(data->m_Device, indices.presentFamily.value(), 0, &m_PresentQueue);
	}

	// Create swapchain
	CreateSwapChain();

	// Create image views
	CreateImageViews();

	//Creating render pass
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_SwapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat(data->m_PhysicalDevice);
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(data->m_Device, &renderPassInfo, nullptr, &data->m_RenderPass) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create render pass!");
		}
	}

	// Create Command Pool
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(data->m_PhysicalDevice, m_Surface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(data->m_Device, &poolInfo, nullptr, &data->m_CommandPool) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create graphics command pool!");
		}
	}

	// Create depth resources
	CreateDepthResources();

	
	// Create command Buffer
	{
		data->m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = data->m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)data->m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(data->m_Device, &allocInfo, data->m_CommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	// Create frame buffers
	CreateFrameBuffers();

	// Create syncronization objects
	{
		m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
		{
			if (vkCreateSemaphore(data->m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(data->m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(data->m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create synchronization objects for a frame!");
			}
		}
	}

}

void VulkanProject::Graphics::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(data->m_PhysicalDevice, m_Surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, m_WindowInstance->m_GLTWwindow);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(data->m_PhysicalDevice, m_Surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(data->m_Device, &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(data->m_Device, m_SwapChain, &imageCount, nullptr);
	m_SwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(data->m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data());

	m_SwapChainImageFormat = surfaceFormat.format;
	data->m_SwapChainExtent = extent;
}

void VulkanProject::Graphics::ClearSwapChain()
{
	vkDestroyImageView(data->m_Device, m_DepthImageView, nullptr);
	vkDestroyImage(data->m_Device, m_DepthImage, nullptr);
	vkFreeMemory(data->m_Device, m_DepthImageMemory, nullptr);

	for (size_t i = 0; i < m_SwapChainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(data->m_Device, m_SwapChainFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
		vkDestroyImageView(data->m_Device, m_SwapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(data->m_Device, m_SwapChain, nullptr);
}

void VulkanProject::Graphics::CreateImageViews()
{
	m_SwapChainImageViews.resize(m_SwapChainImages.size());

	for (size_t i = 0; i < m_SwapChainImages.size(); i++)
	{
		m_SwapChainImageViews[i] = Renderer::CreateImageView(m_SwapChainImages[i], m_SwapChainImageFormat);
	}
}

void VulkanProject::Graphics::CreateFrameBuffers()
{
	m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments = 
		{
			m_SwapChainImageViews[i],
			m_DepthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = data->m_RenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = data->m_SwapChainExtent.width;
		framebufferInfo.height = data->m_SwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(data->m_Device, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void VulkanProject::Graphics::Shutdown()
{
	vkDeviceWaitIdle(data->m_Device);
}

VulkanProject::Graphics::~Graphics()
{
	ClearSwapChain();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(data->m_Device, m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(data->m_Device, m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(data->m_Device, m_InFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(data->m_Device, data->m_CommandPool, nullptr);

	vkDestroyRenderPass(data->m_Device, data->m_RenderPass, nullptr);

	vkDestroyDevice(data->m_Device, nullptr);

	if (enableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	vkDestroyInstance(m_Instance, nullptr);

	m_WindowInstance = nullptr;
}

void VulkanProject::Graphics::Resize()
{

	vkDeviceWaitIdle(data->m_Device);
	ClearSwapChain();
	CreateSwapChain();
	CreateImageViews();
	CreateDepthResources();
	CreateFrameBuffers();
}

void VulkanProject::Graphics::BeginFrame()
{
	vkWaitForFences(data->m_Device, 1, &m_InFlightFences[data->m_CurrentFrame], VK_TRUE, UINT64_MAX);

	m_Result = vkAcquireNextImageKHR(data->m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[data->m_CurrentFrame], VK_NULL_HANDLE, &m_ImageIndex);

	if (m_Result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		Resize();
		return;
	}
	else if (m_Result != VK_SUCCESS && m_Result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(data->m_Device, 1, &m_InFlightFences[data->m_CurrentFrame]);

	vkResetCommandBuffer(data->m_CommandBuffers[data->m_CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(data->m_CommandBuffers[data->m_CurrentFrame], &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = data->m_RenderPass;
	renderPassInfo.framebuffer = m_SwapChainFramebuffers[m_ImageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = data->m_SwapChainExtent;


	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = data->m_ClearColor.color;
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(data->m_CommandBuffers[data->m_CurrentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(data->m_CommandBuffers[data->m_CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, data->m_BoundPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)data->m_SwapChainExtent.width;
	viewport.height = (float)data->m_SwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(data->m_CommandBuffers[data->m_CurrentFrame], 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = data->m_SwapChainExtent;
	vkCmdSetScissor(data->m_CommandBuffers[data->m_CurrentFrame], 0, 1, &scissor);
}

void VulkanProject::Graphics::EndFrame()
{
	//vkCmdDraw(data->m_CommandBuffers[data->m_CurrentFrame], 3, 1, 0, 0);

	vkCmdEndRenderPass(data->m_CommandBuffers[data->m_CurrentFrame]);

	if (vkEndCommandBuffer(data->m_CommandBuffers[data->m_CurrentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[data->m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &data->m_CommandBuffers[data->m_CurrentFrame];

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[data->m_CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(data->m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[data->m_CurrentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { m_SwapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &m_ImageIndex;

	m_Result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

	if (m_Result == VK_ERROR_OUT_OF_DATE_KHR || m_Result == VK_SUBOPTIMAL_KHR || m_WindowInstance->m_Resized)
	{
		m_WindowInstance->m_Resized = false;
		Resize();
	}
	else if (m_Result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	data->m_CurrentFrame = (data->m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

const uint VulkanProject::Renderer::GetCurrentFrame()
{
	return data->m_CurrentFrame;
}

void VulkanProject::Renderer::SetClearColor(glm::vec4& color)
{
	data->m_ClearColor = { color.r, color.g, color.b, color.a };
}

void VulkanProject::Renderer::BindPipeline(const VkPipeline& pipeline, const VkPipelineLayout layout)
{
	data->m_BoundPipeline = pipeline;
	data->m_PipelineLayout = layout;
}

const VkRenderPass VulkanProject::Renderer::GetRenderPass()
{
	return data->m_RenderPass;
}

const VkDevice VulkanProject::Renderer::GetDevice()
{
	return data->m_Device;
}

const VkPhysicalDevice VulkanProject::Renderer::GetPhysicalDevice()
{
	return data->m_PhysicalDevice;
}

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(data->m_PhysicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
	
}

void VulkanProject::Renderer::UploadBuffer(const VkBuffer* buffer, uint32_t sizeOfBuffer)
{
	VkDeviceSize offsets = { 0 };
	data->m_Offset.push_back(offsets);

	vkCmdBindVertexBuffers(data->m_CommandBuffers[data->m_CurrentFrame], 0, 1, buffer, data->m_Offset.data());

	vkCmdDraw(data->m_CommandBuffers[data->m_CurrentFrame], static_cast<uint32_t>(sizeOfBuffer), 1, 0, 0);
}
void VulkanProject::Renderer::UploadIndexedBuffer(const VkBuffer* buffer, VkBuffer indexBuffer, uint32_t sizeOfIndices)
{
	VkDeviceSize offsets = { 0 };
	data->m_Offset.push_back(offsets);

	vkCmdBindVertexBuffers(data->m_CommandBuffers[data->m_CurrentFrame], 0, 1, buffer, data->m_Offset.data());
	vkCmdBindIndexBuffer(data->m_CommandBuffers[data->m_CurrentFrame], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(data->m_CommandBuffers[data->m_CurrentFrame], static_cast<uint32_t>(sizeOfIndices), 1, 0, 0, 0);
}
//void VulkanProject::Renderer::UploadUniformBuffer(std::vector<void*> buffer, UniformBufferObject adata, size_t sizeOfData)
//{
//	memcpy(buffer[data->m_CurrentFrame], &adata, sizeOfData);
//}
void VulkanProject::Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(data->m_Device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(data->m_Device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(data->m_Device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(data->m_Device, buffer, bufferMemory, 0);
	
}
void VulkanProject::Renderer::BindDescriptors(std::vector<VkDescriptorSet> descriptors)
{
	vkCmdBindDescriptorSets(data->m_CommandBuffers[data->m_CurrentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, data->m_PipelineLayout, 0, 1, &descriptors[data->m_CurrentFrame], 0, nullptr);
}
void VulkanProject::Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = data->m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(data->m_Device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(data->m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(data->m_GraphicsQueue);

	vkFreeCommandBuffers(data->m_Device, data->m_CommandPool, 1, &commandBuffer);
}

VkCommandBuffer VulkanProject::Renderer::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = data->m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(data->m_Device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanProject::Renderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(data->m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(data->m_GraphicsQueue);

	vkFreeCommandBuffers(data->m_Device, data->m_CommandPool, 1, &commandBuffer);
}

VkImageView VulkanProject::Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	
	VkImageView imageView;
	if (vkCreateImageView(data->m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image view!");
	}
	
	return imageView;
}

void VulkanProject::Renderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(data->m_Device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(data->m_Device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(data->m_Device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(data->m_Device, image, imageMemory, 0);
}

//VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
//{
//	VkImageViewCreateInfo viewInfo{};
//	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//	viewInfo.image = image;
//	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//	viewInfo.format = format;
//	viewInfo.subresourceRange.aspectMask = aspectFlags;
//	viewInfo.subresourceRange.baseMipLevel = 0;
//	viewInfo.subresourceRange.levelCount = 1;
//	viewInfo.subresourceRange.baseArrayLayer = 0;
//	viewInfo.subresourceRange.layerCount = 1;
//
//	VkImageView imageView;
//	if (vkCreateImageView(data->m_Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
//	{
//		throw std::runtime_error("failed to create image view!");
//	}
//
//	return imageView;
//}