#include "vk.pipeline.h"

namespace vk {
    Pipeline::~Pipeline() {
        std::jthread t0([&] { vkDestroyPipeline(GPU::device, pipeline, nullptr); });
        std::jthread t1([&] { vkDestroyPipelineLayout(GPU::device, layout, nullptr); });
    }
    void Pipeline::bind() {
        VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];

        vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
        vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
    }

    void Pipeline::vkLoadSetLayout(std::vector<VkDescriptorSetLayout>& SetLayout, VkPipelineLayout& layout) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo
        { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(SetLayout.size());
        pipelineLayoutInfo.pSetLayouts = SetLayout.data();
        VK_CHECK_RESULT(vkCreatePipelineLayout(GPU::device, &pipelineLayoutInfo, nullptr, &layout));
    }

    std::vector<VkPipelineShaderStageCreateInfo> Pipeline::stageInfo(std::vector<Shader>& shaders) {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.pName = "main";

        std::vector<VkPipelineShaderStageCreateInfo> shader_stages(shaders.size());
        for (int i = 0; i < shaders.size(); i++) {
            stageInfo.module = shaders[i].shaderModule;
            stageInfo.stage = shaders[i].shaderStage;
            shader_stages[i] = stageInfo;
        }

        return shader_stages;
    }

    VkPipelineViewportStateCreateInfo Pipeline::viewportState(uint32_t viewportCount, uint32_t scissorCount, VkViewport* pViewports, VkRect2D* pScissors) {
        VkPipelineViewportStateCreateInfo viewport_state
        { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport_state.viewportCount = viewportCount;
        viewport_state.scissorCount = scissorCount;
        viewport_state.pViewports = pViewports;
        viewport_state.pScissors = pScissors;

        return viewport_state;
    }
    VkPipelineRasterizationStateCreateInfo Pipeline::rasterState(VkPolygonMode drawType, VkCullModeFlags cullType, VkFrontFace frontFace) {
        VkPipelineRasterizationStateCreateInfo rasterizer
        { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = drawType;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = cullType;
        rasterizer.frontFace = frontFace;
        rasterizer.depthBiasEnable = VK_FALSE;
        return rasterizer;
    }
    VkPipelineMultisampleStateCreateInfo Pipeline::msaaState(bool sampleShading, float minSampleShading) {
        VkPipelineMultisampleStateCreateInfo msaaInfo
        { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        msaaInfo.sampleShadingEnable = sampleShading; // enable sample shading in the pipeline
        msaaInfo.minSampleShading = minSampleShading; // min fraction for sample shading; closer to one is smoother
        msaaInfo.rasterizationSamples = GPU::msaaSamples;
        return msaaInfo;
    }
    VkPipelineDepthStencilStateCreateInfo Pipeline::depthStencilState() {
        VkPipelineDepthStencilStateCreateInfo depthStencil
        { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;

        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional

        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional
        return depthStencil;
    }
    VkPipelineColorBlendStateCreateInfo Pipeline::colorBlendState(VkPipelineColorBlendAttachmentState& colorBlendAttachment, VkBool32 logicOpEnable, VkLogicOp logicOp) {
        VkPipelineColorBlendStateCreateInfo colorBlending
        { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.logicOpEnable = logicOpEnable;
        colorBlending.logicOp = logicOp;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;
        return colorBlending;
    }
    VkPipelineDynamicStateCreateInfo Pipeline::dynamicState(std::vector<VkDynamicState>& dynamicStates) {
        VkPipelineDynamicStateCreateInfo dynamic_state
        { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamic_state.pDynamicStates = dynamicStates.data();
        return dynamic_state;
    }


}