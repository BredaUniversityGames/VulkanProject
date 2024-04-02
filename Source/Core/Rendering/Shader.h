#pragma once
#include <vector>
#include <string>
#include "Core/Includes.h"

namespace VulkanProject
{
  


    class Graphics;
    struct PipelineDesc
    {
        std::string vertexShaderPath;
        std::string fragmentShaderPath;

    };
    class GraphicsPipeline
    {
    public:
        GraphicsPipeline(PipelineDesc& desc);
        ~GraphicsPipeline();
        void Bind();
    private:
        VkShaderModule createShaderModule(const std::vector<char>& code);

        VkPipelineLayout m_PipelineLayout;
        VkPipeline m_GraphicsPipeline;
       // VkRenderPass m_RenderPass;
    };
}
