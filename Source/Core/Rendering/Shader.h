#pragma once
#include <vector>
#include <string>
#include "Core/Includes.h"
#include "Core/Defines.h"
#include <array>

namespace VulkanProject
{
  


    class Graphics;
    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
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
    struct PipelineDesc
    {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;

		//std::vector<Vertex> vertex;
		
    };

    
    struct UniformBuffer
    {

        UniformBuffer(VkDescriptorSetLayout& layout);
        ~UniformBuffer();
     
        std::vector<VkBuffer> m_UniformBuffers;
        std::vector<VkDeviceMemory> m_UniformBuffersMemory;
        std::vector<void*> m_UniformBuffersMapped;

        VkDescriptorPool m_DescriptorPool;
        std::vector<VkDescriptorSet> m_DescriptorSets;
    };

    struct UniformBufferObject;
    class GraphicsPipeline
    {
    public:
        GraphicsPipeline(PipelineDesc& desc);
        ~GraphicsPipeline();
        void Bind();
        void UpdateBuffers(UniformBufferObject& ubo);
    private:
        VkShaderModule createShaderModule(const std::vector<char>& code);
        void CreateTextureSamplers();

        VkDescriptorSetLayout m_DescriptorSetLayout;
        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_GraphicsPipeline;
        
       // VkRenderPass m_RenderPass;
        UniformBuffer* m_Uniformbuffer;

        VkSampler m_TextureSampler;

    };


    class Mesh
    {
    public:
        Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices);
        ~Mesh();
        void Draw();
    private:
        VkBuffer m_VertexBuffer;
        VkDeviceMemory m_VertexBufferMemory;

        VkBuffer m_IndexBuffer;
        VkDeviceMemory m_IndexBufferMemory;

        //size_t sizeOfVertices;
        size_t sizeOfIndices;
    };
}
