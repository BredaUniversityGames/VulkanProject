#include "Graphics.h"
#include "Core/Window.h"

#include <iostream>
#include <vector>
#include <set>
#include <fstream>

#include "HelperFunctions.h"



struct RenderData
{
	VkRenderPass m_RenderPass;
	VkDevice m_Device = nullptr;
	VkPipeline m_BoundPipeline;
	VkClearValue m_ClearColor = { 0.f,0.f,0.f,0.f };
};
static RenderData* data;


VulkanProject::Graphics::Graphics(Window* window) : m_WindowInstance(window)
{
	data = new RenderData();
	
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
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
			createInfo.ppEnabledLayerNames = g_validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
	}

	// Set up validation layer
	{
		if (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
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
				m_PhysicalDevice = device;
				break;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	// Create a logical device
	{
		QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice, m_Surface);

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

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(g_deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();

		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
			createInfo.ppEnabledLayerNames = g_validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &data->m_Device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		vkGetDeviceQueue(data->m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
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

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		if (vkCreateRenderPass(data->m_Device, &renderPassInfo, nullptr, &data->m_RenderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	// Create frame buffers
	CreateFrameBuffers();

	// Create Command Pool
	{
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_PhysicalDevice, m_Surface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		if (vkCreateCommandPool(data->m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	// Create command Buffer
	{
		m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

		if (vkAllocateCommandBuffers(data->m_Device, &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

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
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_PhysicalDevice, m_Surface);

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

	QueueFamilyIndices indices = findQueueFamilies(m_PhysicalDevice, m_Surface);
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
	m_SwapChainExtent = extent;
}

void VulkanProject::Graphics::ClearSwapChain()
{
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
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_SwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = m_SwapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(data->m_Device, &createInfo, nullptr, &m_SwapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void VulkanProject::Graphics::CreateFrameBuffers()
{
	m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

	for (size_t i = 0; i < m_SwapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			m_SwapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = data->m_RenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_SwapChainExtent.width;
		framebufferInfo.height = m_SwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(data->m_Device, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS) {
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

	vkDestroyCommandPool(data->m_Device, m_CommandPool, nullptr);

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

void VulkanProject::Graphics::DrawFrame()
{
	vkWaitForFences(data->m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(data->m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		Resize();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(data->m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

	vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
	recordCommandBuffer(m_CommandBuffers[m_CurrentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];

	VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) 
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

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_WindowInstance->m_Resized)
	{
		m_WindowInstance->m_Resized = false;
		Resize();
	}
	else if (result != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanProject::Graphics::Resize()
{
	vkDeviceWaitIdle(data->m_Device);
	ClearSwapChain();
	CreateSwapChain();
	CreateImageViews();
	CreateFrameBuffers();
}


void VulkanProject::Graphics::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = data->m_RenderPass;
	renderPassInfo.framebuffer = m_SwapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_SwapChainExtent;

	
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &data->m_ClearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, data->m_BoundPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_SwapChainExtent.width;
	viewport.height = (float)m_SwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = m_SwapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VulkanProject::Renderer::SetClearColor(glm::vec4& color)
{
	data->m_ClearColor = { color.r, color.g, color.b, color.a };
}

void VulkanProject::Renderer::BindPipeline(const VkPipeline& pipeline)
{
	data->m_BoundPipeline = pipeline;
}

const VkRenderPass VulkanProject::Renderer::GetRenderPass()
{
	return data->m_RenderPass;
}

const VkDevice VulkanProject::Renderer::GetDevice()
{
	return data->m_Device;
}

