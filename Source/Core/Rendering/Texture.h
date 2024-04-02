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
		const VkImageView GetImageview() const {  return m_TextureImageView; }
	private:
		
		VkImage m_TextureImage;
		VkDeviceMemory m_TextureImageMemory;
		VkImageView m_TextureImageView;
	};
}

