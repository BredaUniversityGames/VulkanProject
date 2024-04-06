#include "Texture.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

#include <stdexcept>
#include "Graphics.h"
#include "Shader.h"
namespace VulkanProject
{
void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
}

VulkanProject::Texture::Texture(std::string filepath)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) 
    {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    Renderer::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(Renderer::GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(Renderer::GetDevice(), stagingBufferMemory);

    stbi_image_free(pixels);

    Renderer::CreateImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);

    TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(Renderer::GetDevice(), stagingBuffer, nullptr);
    vkFreeMemory(Renderer::GetDevice(), stagingBufferMemory, nullptr);

	m_TextureImageView = Renderer::CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB);

}

VulkanProject::Texture::~Texture()
{
	vkDestroyImage(Renderer::GetDevice(), m_TextureImage, nullptr);
	vkFreeMemory(Renderer::GetDevice(), m_TextureImageMemory, nullptr);
	vkDestroyImageView(Renderer::GetDevice(), m_TextureImageView, nullptr);
}

void VulkanProject::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = Renderer::BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	Renderer::EndSingleTimeCommands(commandBuffer);
}
void VulkanProject::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = Renderer::BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent =
	{
			width,
			height,
			1
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	Renderer::EndSingleTimeCommands(commandBuffer);
}

VulkanProject::Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
{
	// vertex buffer
	{

		//sizeOfVertices = vertices.size();

		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Renderer::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(Renderer::GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(Renderer::GetDevice(), stagingBufferMemory);

		Renderer::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory);

		Renderer::CopyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

		vkDestroyBuffer(Renderer::GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(Renderer::GetDevice(), stagingBufferMemory, nullptr);
	}

	// index buffer
	{
		sizeOfIndices = indices.size();
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		Renderer::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(Renderer::GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(Renderer::GetDevice(), stagingBufferMemory);

		Renderer::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_IndexBuffer, m_IndexBufferMemory);

		Renderer::CopyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

		vkDestroyBuffer(Renderer::GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(Renderer::GetDevice(), stagingBufferMemory, nullptr);
	}
}

void VulkanProject::Mesh::Draw(glm::mat4 model)
{

	VkBuffer vertexBuffers[] = { m_VertexBuffer };
	Renderer::UploadIndexedBuffer(vertexBuffers, m_IndexBuffer, sizeOfIndices);
}

VulkanProject::Mesh::~Mesh()
{
	vkDestroyBuffer(Renderer::GetDevice(), m_IndexBuffer, nullptr);
	vkFreeMemory(Renderer::GetDevice(), m_IndexBufferMemory, nullptr);

	vkDestroyBuffer(Renderer::GetDevice(), m_VertexBuffer, nullptr);
	vkFreeMemory(Renderer::GetDevice(), m_VertexBufferMemory, nullptr);
}

void CalculateNormal(std::vector<VulkanProject::Vertex>& vertices, std::vector<unsigned int>& indices)
{
	for (unsigned int i = 0; i < indices.size(); i += 3)
	{
		auto& vertex1 = vertices[indices[i + 0]];
		auto& vertex2 = vertices[indices[i + 1]];
		auto& vertex3 = vertices[indices[i + 2]];

		glm::vec3 positionVertex1 = { vertex1.pos[0], vertex1.pos[1], vertex1.pos[2] };
		glm::vec3 positionVertex2 = { vertex2.pos[0], vertex2.pos[1], vertex2.pos[2] };
		glm::vec3 positionVertex3 = { vertex3.pos[0], vertex3.pos[1], vertex3.pos[2] };


		glm::vec3 v1v2 = positionVertex2 - positionVertex1;
		glm::vec3 v1v3 = positionVertex3 - positionVertex1;

		glm::vec3 normal;
		normal = glm::cross(v1v2, v1v3);
		normal = glm::normalize(normal);

		glm::vec3 normalVertex1 = { vertex1.normal[0], vertex1.normal[1], vertex1.normal[2] };
		glm::vec3 normalVertex2 = { vertex2.normal[0], vertex2.normal[1], vertex2.normal[2] };
		glm::vec3 normalVertex3 = { vertex3.normal[0], vertex3.normal[1], vertex3.normal[2] };

		normalVertex1 += normal;
		normalVertex2 += normal;
		normalVertex3 += normal;

		normalVertex1 = glm::normalize(normalVertex1);
		normalVertex2 = glm::normalize(normalVertex2);
		normalVertex3 = glm::normalize(normalVertex3);

		vertex1.normal[0] = normalVertex1.x;
		vertex1.normal[1] = normalVertex1.y;
		vertex1.normal[2] = normalVertex1.z;

		vertex2.normal[0] = normalVertex2.x;
		vertex2.normal[1] = normalVertex2.y;
		vertex2.normal[2] = normalVertex2.z;

		vertex3.normal[0] = normalVertex3.x;
		vertex3.normal[1] = normalVertex3.y;
		vertex3.normal[2] = normalVertex3.z;


	}
}
bool GetData(std::vector<VulkanProject::Vertex>& vertices, const tinygltf::Primitive& primitive, const tinygltf::Model& model, std::string type, int numFloats, size_t vertexCount)
{
	int accessorIndex = -1;

	if (primitive.attributes.find(type.c_str()) != primitive.attributes.end())
	{
		accessorIndex = primitive.attributes.at(type.c_str());
	}
	else
	{
		return false;
	}
	const auto& Accessor = model.accessors[accessorIndex];
	const auto& BufferView = model.bufferViews[Accessor.bufferView];
	const auto& Buffer = model.buffers[BufferView.buffer];

	//vertices.resize(vertexCount);

	int Stride = Accessor.ByteStride(BufferView);
	if(!vertexCount == Accessor.count)
	{
		throw std::runtime_error("Accessor does not allign with positionAccessor");
	}

	for (int i = 0; i < vertexCount; i++)
	{
		size_t index = BufferView.byteOffset + Accessor.byteOffset + i * Stride;

		if (type == "TEXCOORD_0")
		{
			vertices[i].texCoord.x = *(float*)(&Buffer.data[index + sizeof(float) * 0]);
			vertices[i].texCoord.y = *(float*)(&Buffer.data[index + sizeof(float) * 1]);
		}
		else if (type == "POSITION")
		{
			vertices[i].pos.x = *(float*)(&Buffer.data[index + sizeof(float) * 0]);
			vertices[i].pos.y = *(float*)(&Buffer.data[index + sizeof(float) * 1]);
			vertices[i].pos.z = *(float*)(&Buffer.data[index + sizeof(float) * 2]);
		}
		else if (type == "NORMAL")
		{
			vertices[i].normal.x = *(float*)(&Buffer.data[index + sizeof(float) * 0]);
			vertices[i].normal.y = *(float*)(&Buffer.data[index + sizeof(float) * 1]);
			vertices[i].normal.z = *(float*)(&Buffer.data[index + sizeof(float) * 2]);
		}
		else if (type == "TANGENT")
		{/*
			vertices[i].tangents[0] = *(float*)(&Buffer.data[index + sizeof(float) * 0]);
			vertices[i].tangents[1] = *(float*)(&Buffer.data[index + sizeof(float) * 1]);
			vertices[i].tangents[2] = *(float*)(&Buffer.data[index + sizeof(float) * 2]);*/

		}



	}

	return true;
}

enum class eTextureTypes
{
	Diffuse = 0,
	Normal = 1,
	Metalic_Roughness = 2
};

std::string GetTexturePathforPrimitive(const tinygltf::Primitive& primitive, const tinygltf::Model& model, std::string filepath, eTextureTypes type)
{
	std::filesystem::path fullPath = filepath;
	std::string textureName;
	int imageIndex = -1;
	int textureIndex = -1;
	switch (type)
	{
	case eTextureTypes::Diffuse:
	{

		textureIndex = model.materials[primitive.material].pbrMetallicRoughness.baseColorTexture.index;

		break;
	}
	case eTextureTypes::Normal:
	{

		textureIndex = model.materials[primitive.material].normalTexture.index;

		break;
	}
	case eTextureTypes::Metalic_Roughness:
	{


		textureIndex = model.materials[primitive.material].pbrMetallicRoughness.metallicRoughnessTexture.index;

		break;
	}
	}


	imageIndex = model.textures[textureIndex].source;
	textureName = model.images[imageIndex].uri;
	std::string texturePath = fullPath.parent_path().string() + "/" + textureName;

	
	return texturePath;
}
VulkanProject::Model::Model(std::string path)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	//bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)

	if (!warn.empty()) 
	{
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty())
	{
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) 
	{
		throw std::runtime_error("Failed to parse glTF");
	}

	for (const auto& mesh : model.meshes)
	{
		std::vector<Primitive> primitives;
		for (const auto& primtive : mesh.primitives)
		{
			std::vector<unsigned int> indices;
			//calculating indices
			{
				const auto& accessor = model.accessors[primtive.indices];
				const auto& bufferView = model.bufferViews[accessor.bufferView];
				const auto& buffer = model.buffers[bufferView.buffer];

				indices.resize(accessor.count);
				for (int i = 0; i < accessor.count; i++)
				{
					size_t index = bufferView.byteOffset + accessor.byteOffset;

					if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT)
					{
						indices[i] = static_cast<unsigned int>(*(short*)(&buffer.data[index + i * sizeof(short)]));

					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
					{
						indices[i] = static_cast<unsigned int>(*(unsigned short*)(&buffer.data[index + i * sizeof(unsigned short)]));

					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_INT)
					{
						indices[i] = static_cast<unsigned int>(*(int*)(&buffer.data[index + i * sizeof(int)]));

					}
					else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
					{
						indices[i] = static_cast<unsigned int>(*(unsigned int*)(&buffer.data[index + i * sizeof(unsigned int)]));

					}
					else throw std::runtime_error("unsupported indices type");

				}
				//vertexData.numIndices = static_cast<unsigned int>(accessor.count);
			}


			std::vector<Vertex> vertices;
			//calcualting vertices
			{

				const auto& positionAccessor = model.accessors[primtive.attributes.at("POSITION")];
				size_t vertexCount = positionAccessor.count;
				vertices.resize(vertexCount);
				GetData(vertices, primtive, model, "POSITION", 3, vertexCount);

				GetData(vertices, primtive, model, "TEXCOORD_0", 2, vertexCount);

				if (!GetData(vertices, primtive, model, "NORMAL", 3, vertexCount))
				{
					CalculateNormal(vertices, indices);
				};

				/*if (!GetData(vertices, primtive, model, "TANGENT", 3, vertexCount))
				{
					CalculateTangent(vertices, indices);
				}*/



				
				//vertexData.numVertices = static_cast<unsigned int>(vertexCount);
				//vertexData.vertexStrideInBytes = sizeof(Vertex);
			}

			//textures
			if (primtive.material != -1)
			{

				if (model.materials[primtive.material].pbrMetallicRoughness.baseColorTexture.index != -1)
				{

					std::string texturePath = GetTexturePathforPrimitive(primtive, model, path, eTextureTypes::Diffuse);


					if (model.materials[primtive.material].normalTexture.index != -1)
					{

						std::string normalPath = GetTexturePathforPrimitive(primtive, model, path, eTextureTypes::Normal);

						//primitives.push_back({ new Mesh(vertices, indices), new Texture(texturePath), Texture(normalPath), nullptr });
						primitives.push_back({ new Mesh(vertices, indices), new Texture(texturePath)});
						continue;
					}


					primitives.push_back({ new Mesh(vertices, indices), new Texture(texturePath)});
					continue;
				}

			}
			//when there are no textures we return a nullptr
			primitives.push_back({ new Mesh(vertices, indices),nullptr });
		}
		m_Meshes.push_back(primitives);
	}

	m_Nodes.resize(model.nodes.size());
	for (int i = 0; i < model.nodes.size(); i++)
	{
		for (int j = 0; j < model.nodes[i].children.size(); j++)
			m_Nodes[i].children.push_back(model.nodes[i].children[j]);

		m_Nodes[i].mesh = model.nodes[i].mesh;

		glm::mat4 transform;

		if (model.nodes[i].matrix.size() > 0)
		{
			//float value[16] = { 0.f };
			for (int j = 0; j < model.nodes[i].matrix.size(); j++)
			{
				//value[j] = static_cast<float>(model.nodes[i].matrix[j]);
				transform = model.nodes[i].matrix[j];
			}
			//transform = DirectX::SimpleMath::Matrix(value);
		}
		// Asuming that scale and rotation are present when translation is
		else
		{
			glm::vec3 translation = { 0.f,0.f,0.f };
			glm::quat rotation;
			glm::vec3 scale = { 1.f,1.f,1.f };
			if (model.nodes[i].translation.size() > 0)
			{
				auto trans = model.nodes[i].translation;
				translation = { static_cast<float>(trans[0]), static_cast<float>(trans[1]), static_cast<float>(trans[2]) };
			}
			if (model.nodes[i].rotation.size() > 0)
			{
				auto rot = model.nodes[i].rotation;
				rotation = { static_cast<float>(rot[0]), static_cast<float>(rot[1]), static_cast<float>(rot[2]), static_cast<float>(rot[3]) };
			}
			if (model.nodes[i].scale.size() > 0)
			{

				scale = { static_cast<float>(model.nodes[i].scale[0]), static_cast<float>(model.nodes[i].scale[1]), static_cast<float>(model.nodes[i].scale[2]) };

			}

			glm::mat4 translationM = glm::translate(translationM, translation);
			glm::mat4 rotationM = glm::toMat4(rotation);
			glm::mat4 scaleM = glm::scale(scaleM, scale);
			transform = transform * rotationM * scaleM;
		
		}
		m_Nodes[i].transform = transform;
	}
	for (int i = 0; i < model.scenes[model.defaultScene].nodes.size(); i++)
	{
		m_RootNodes.push_back(model.scenes[model.defaultScene].nodes[i]);
	}
}

VulkanProject::Model::~Model()
{
}
void VulkanProject::Model::DrawNode(int index, glm::mat4 parentTransform, GraphicsPipeline& pipeline)
{
	const auto& node = m_Nodes[index];
	auto transform = node.transform * parentTransform;
	if (node.mesh >= 0)
	{
		for (const auto& primitve : m_Meshes[node.mesh])
		{
			//making sure that only present textures are bound
			if (primitve.texture != nullptr)
			{
				pipeline.UpdateDesctiptorSets(*primitve.texture);
				//primitve.texture->Bind(0);
			}
			/*if (primitve.normalTexture != nullptr)
			{
				primitve.normalTexture->Bind(1);
			}
			if (primitve.metalic_roughnessTexture != nullptr)
			{
				primitve.metalic_roughnessTexture->Bind(1);

			}*/
			primitve.mesh->Draw(transform);
		}
	}
	for (int i = 0; i < node.children.size(); i++)
	{
		DrawNode(node.children[i], transform, pipeline);
	}

}
void VulkanProject::Model::Draw(glm::mat4 modelmatrix, GraphicsPipeline& pipeline)
{
	for (int i = 0; i < m_RootNodes.size(); i++)
	{
		DrawNode(m_RootNodes[i], modelmatrix, pipeline);
	}
}


