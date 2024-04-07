#pragma once
#include <vector>
#include <string>
#include "Core/Includes.h"
#include "Core/Defines.h"
#include <unordered_map>

namespace VulkanProject
{
    

    class Graphics;
   
    struct PipelineDesc
    {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;

		//std::vector<Vertex> vertex;
		
    };

    struct UniformBufferObject;
    class Texture;

    class GraphicsPipeline
    {
    public:
        GraphicsPipeline(PipelineDesc& desc);
        ~GraphicsPipeline();
        void Bind();
        void UpdateBuffers(UniformBufferObject& ubo);
        void UploadModelBuffer(glm::mat4 model);
        void BindData();
        void UpdateDesctiptorSets(std::vector<Texture*> textures);
    
    private:
        VkShaderModule createShaderModule(const std::vector<char>& code);
        void CreateTextureSamplers();

        VkDescriptorSetLayout m_DescriptorSetLayout;
        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_GraphicsPipeline;
        
        std::vector<VkBuffer> m_UniformBuffers;
        std::vector<VkDeviceMemory> m_UniformBuffersMemory;
        std::vector<void*> m_UniformBuffersMapped;


        std::vector<VkBuffer> m_ModelBuffer;
        std::vector<VkDeviceMemory> m_ModelBuffersMemory;
        std::vector<void*> m_ModelBuffersMapped;

        VkDescriptorPool m_DescriptorPool;
        std::vector<VkDescriptorSet> m_DescriptorSets;
       // VkRenderPass m_RenderPass;
       
        VkSampler m_TextureSampler;
       
    };

   
}
