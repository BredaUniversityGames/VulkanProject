#pragma once
#include "Core/Includes.h"
#include <string>
#include <array>
#include <vector>

namespace VulkanProject
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;
        glm::vec3 normal;
        //glm::vec3 normal;
        //glm::vec3 tangent;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, normal);

            /*attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(Vertex, tangent);*/

            return attributeDescriptions;
        }
    };

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

    class Mesh
    {
    public:
        Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);
        ~Mesh();
        void Draw(glm::mat4 model);
    private:
        VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_VertexBufferMemory;

        VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_IndexBufferMemory;

        //size_t sizeOfVertices;
        uint32_t sizeOfIndices;
    };
    class GraphicsPipeline;
    class Model
    {
    public:
        Model(std::string path);
        ~Model();
        void Draw(glm::mat4 modelmatrix, GraphicsPipeline& pipeline);
    private:
  
        void DrawNode(int index,glm::mat4 parentTransform, GraphicsPipeline& pipeline);
        struct Primitive
        {
           Mesh* mesh;
           Texture* texture;
           //Texture normalTexture;
           //Texture metalic_roughnessTexture;
        };
        Primitive LoadPrimitive();

        struct Node
        {
           glm::mat4 transform;
           std::vector<unsigned int> children;
           int mesh = -1;
        };

        std::vector<Node> m_Nodes;
        std::vector<unsigned int> m_RootNodes;

        std::vector<std::vector<Primitive>> m_Meshes;
    };
}

