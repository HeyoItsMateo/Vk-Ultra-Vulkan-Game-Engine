#pragma once
#ifndef hGraphics
#define hGraphics

#include "vk.swapchain.h"

#include "vk.pipeline.h"
#include "vk.shader.h"

#include "descriptors.h"
#include "vk.primitives.h"

namespace vk {    
    /*
    template<typename primitiveType, VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL>
    struct GraphicsPPL_ : Pipeline {
        template<uint32_t nShaders, uint32_t setCount>
        GraphicsPPL_(Shader(&shaders)[nShaders], Descriptor(&descriptors)[setCount])
        {
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            size = setCount;
            pSets = descriptors;
            
            //sets = descSets;
            testDescriptorSets(descriptors, size);
            vkCreatePipeline(stageInfo(shaders, nShaders));
        }
        void bind() override {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];
            sets.resize(size);
            for (int i = 0; i < size; i++) {
                sets[i] = pSets[i].Sets[SwapChain::currentFrame];
            }
            vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, 0, size, sets.data(), 0, nullptr);
            vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
        }
        uint32_t size;
        Descriptor* pSets;
    private:
        void testDescriptorSets(Descriptor* descriptors, uint32_t setCount) {
            std::vector<VkDescriptorSetLayout> SetLayouts(setCount);
            for (int i = 0; i < setCount; i++) {
                SetLayouts[i] = descriptors[i].SetLayout;
            }
            VkPipelineLayoutCreateInfo pipelineLayoutInfo
            { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
            pipelineLayoutInfo.setLayoutCount = size;
            pipelineLayoutInfo.pSetLayouts = SetLayouts.data();

            VK_CHECK_RESULT(vkCreatePipelineLayout(GPU::device, &pipelineLayoutInfo, nullptr, &layout));
        }

        std::vector<VkPipelineShaderStageCreateInfo> stageInfo(Shader* shaders, uint32_t size) {
            VkPipelineShaderStageCreateInfo stageInfo{};
            stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageInfo.pName = "main";

            std::vector<VkPipelineShaderStageCreateInfo> shader_stages{ size, stageInfo };
            for (int i = 0; i < size; i++) {
                shader_stages[i].module = shaders[i].shaderModule;
                shader_stages[i].stage = shaders[i].shaderStage;
            }

            return shader_stages;
        }
        void vkCreatePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderStages) {

            VkPipelineVertexInputStateCreateInfo vertexInputInfo = primitiveType::vertexInput();
            VkPipelineInputAssemblyStateCreateInfo vertexAssemblyInfo = primitiveType::inputAssembly();

            std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicStateInfo = dynamicState(dynamicStates);
            VkPipelineViewportStateCreateInfo viewportInfo = viewportState(1, 1);

            VkPipelineRasterizationStateCreateInfo rasterInfo = rasterState(polygonMode);

            VkPipelineMultisampleStateCreateInfo msaaInfo = msaaState(VK_TRUE, 0.2f);

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
            VkPipelineColorBlendStateCreateInfo colorBlendInfo = colorBlendState(colorBlendAttachment, VK_FALSE);

            VkPipelineDepthStencilStateCreateInfo depthStencilInfo = depthStencilState();

            VkGraphicsPipelineCreateInfo pipelineInfo
            { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
            pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.layout = layout;

            pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineInfo.pStages = shaderStages.data();

            pipelineInfo.renderPass = SwapChain::renderPass;
            pipelineInfo.subpass = 0;

            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &vertexAssemblyInfo;

            pipelineInfo.pDynamicState = &dynamicStateInfo;
            pipelineInfo.pViewportState = &viewportInfo;

            pipelineInfo.pRasterizationState = &rasterInfo;
            pipelineInfo.pMultisampleState = &msaaInfo;
            pipelineInfo.pColorBlendState = &colorBlendInfo;
            pipelineInfo.pDepthStencilState = &depthStencilInfo;

            VK_CHECK_RESULT(vkCreateGraphicsPipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
        }
    };
    */
    template<typename primitiveType, VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL>
    struct GraphicsPPL : Pipeline {
        template<uint32_t size>
        GraphicsPPL(Shader(&shaders)[size], std::vector<VkDescriptorSet> const& descSets, std::vector<VkDescriptorSetLayout>& SetLayout);
    private:
        std::vector<VkPipelineShaderStageCreateInfo> stageInfo(Shader* shaders, uint32_t size);
        void vkCreatePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderStages);
    };

    template<typename primitiveType, VkPolygonMode polygonMode>
    struct derivativePPL : Pipeline {
        derivativePPL(VkPipeline& parentPPL, std::vector<Shader> const& shaders, std::vector<VkDescriptorSet>& descSets, std::vector<VkDescriptorSetLayout>& setLayouts)
        {
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sets = descSets;
            vkLoadSetLayout(setLayouts);
            vkCreatePipeline(parentPPL, stageInfo(shaders));
        }
    protected:
        void vkCreatePipeline(VkPipeline& parentPPL, std::vector<VkPipelineShaderStageCreateInfo> shaderStages) {
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = primitiveType::vertexInput();
            VkPipelineInputAssemblyStateCreateInfo inputAssembly = primitiveType::inputAssembly();

            std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicStateInfo = dynamicState(dynamicStates);

            VkPipelineViewportStateCreateInfo viewportState
            { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineRasterizationStateCreateInfo rasterInfo = rasterState(polygonMode);
            VkPipelineMultisampleStateCreateInfo msaaInfo = msaaState(VK_TRUE, 0.2f);
            VkPipelineColorBlendStateCreateInfo colorBlendInfo = colorBlendState(colorBlendAttachment, VK_FALSE);
            VkPipelineDepthStencilStateCreateInfo depthStencilInfo = depthStencilState();

            VkGraphicsPipelineCreateInfo pipelineInfo
            { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
            pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
            pipelineInfo.basePipelineHandle = parentPPL;
            pipelineInfo.basePipelineIndex = -1;
            pipelineInfo.layout = layout;

            pipelineInfo.renderPass = SwapChain::renderPass;
            pipelineInfo.subpass = 0;

            pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineInfo.pStages = shaderStages.data();

            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pDynamicState = &dynamicStateInfo;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterInfo;
            pipelineInfo.pMultisampleState = &msaaInfo;
            pipelineInfo.pColorBlendState = &colorBlendInfo;
            pipelineInfo.pDepthStencilState = &depthStencilInfo;

            VK_CHECK_RESULT(vkCreateGraphicsPipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
        }
    };
    
    template<typename primitiveType, VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL>
    struct GraphicsPPL_ : Pipeline {
        template<uint32_t size>
        GraphicsPPL_(Shader_(&shaders)[size], std::vector<VkDescriptorSet> const& descSets, std::vector<VkDescriptorSetLayout>& SetLayout);
    private:
        std::vector<VkPipelineShaderStageCreateInfo> stageInfo(Shader_* shaders, uint32_t size);
        void vkCreatePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderStages);
    };
}

#include "vk.graphics.ipp"

#endif