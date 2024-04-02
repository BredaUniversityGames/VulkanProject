#pragma once
#include "Core/Includes.h"
#include <string>

namespace VulkanProject
{
	class Texture
	{
	public: 
		Texture(std::string filepath);
		~Texture();
	private:
		
		VkImage m_TextureImage;
		VkDeviceMemory m_TextureImageMemory;
		VkImageView m_TextureImageView;
	};
}

