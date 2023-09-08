#pragma once
#include "vk.swapchain.h"
#include "vk.cpu.h"

#include "Primitives.h"

#include <thread>
#include <format>
#include <set>

namespace vk {
    struct Shader {
        VkShaderModule shaderModule;
        VkPipelineShaderStageCreateInfo stageInfo
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        Shader(const std::string& filename, VkShaderStageFlagBits shaderStage) {
            try {
                auto shaderCode = readFile(".\\shaders\\" + filename + ".spv");
                createShaderModule(shaderCode, filename);
            }
            catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
            }
            stageInfo.stage = shaderStage;
            stageInfo.module = shaderModule;
            stageInfo.pName = "main";
        }
        ~Shader() {
            vkDestroyShaderModule(GPU::device, shaderModule, nullptr);
        }
    private:
        void createShaderModule(const std::vector<char>& code, const std::string& filename) {
            VkShaderModuleCreateInfo createInfo
            { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            if (vkCreateShaderModule(GPU::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create " + filename + "!");
            }
        }
        // File Reader
        static std::vector<char> readFile(const std::string& filename) {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error(std::format("failed to open {}!", filename));
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);

            file.close();

            return buffer;
        }
    };

    struct Pipeline{
    public:
        VkPipeline pipeline;
        VkPipelineLayout layout;
        void bind(std::vector<VkDescriptorSet>& Sets) {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];

            vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, 0, static_cast<uint32_t>(Sets.size()), Sets.data(), 0, nullptr);
            vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
        }
    protected:
        VkPipelineBindPoint bindPoint;
        void vkLoadSetLayout(std::vector<VkDescriptorSetLayout>& SetLayout) {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo
            { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(SetLayout.size());
            pipelineLayoutInfo.pSetLayouts = SetLayout.data();

            if (vkCreatePipelineLayout(GPU::device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
        }
        static VkPipelineRasterizationStateCreateInfo rasterState(VkPolygonMode drawType, VkCullModeFlags cullType = VK_CULL_MODE_BACK_BIT, VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE) {
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
        static VkPipelineMultisampleStateCreateInfo msaaState(bool sampleShading, float minSampleShading) {
            VkPipelineMultisampleStateCreateInfo msaaInfo
            { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            msaaInfo.sampleShadingEnable = sampleShading; // enable sample shading in the pipeline
            msaaInfo.minSampleShading = minSampleShading; // min fraction for sample shading; closer to one is smoother
            msaaInfo.rasterizationSamples = GPU::msaaSamples;
            return msaaInfo;
        }
        static VkPipelineDepthStencilStateCreateInfo depthStencilState() {
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
        static VkPipelineColorBlendStateCreateInfo colorBlendState(VkBool32 logicOpEnable, VkLogicOp logicOp = VK_LOGIC_OP_COPY) {
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

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
        static VkPipelineDynamicStateCreateInfo dynamicState(std::vector<VkDynamicState>& dynamicStates) {
            VkPipelineDynamicStateCreateInfo dynamic_state
            { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
            dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamic_state.pDynamicStates = dynamicStates.data();
            return dynamic_state;
        }
    };

    template<typename T>
    struct GraphicsPPL : Pipeline {
        GraphicsPPL(std::set<Shader>& shaders, std::vector<VkDescriptorSetLayout>& setLayouts)
        {//TODO: Rewrite 'VkCreatePipeline' to change based on needed vertex input,
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            vkLoadSetLayout(setLayouts);
            vkCreatePipeline(shaders);
        }
        ~GraphicsPPL() {
            std::jthread t0([&] { vkDestroyPipeline(GPU::device, pipeline, nullptr); });
            std::jthread t1([&] { vkDestroyPipelineLayout(GPU::device, layout, nullptr); });
        }
    protected:
        void vkCreatePipeline(std::set<Shader>& shaders) {
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
            for (Shader shader : shaders) {
                shaderStages.push_back(shader.stageInfo);
            }
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = T::vkCreateVertexInput();

            VkPipelineInputAssemblyStateCreateInfo inputAssembly
            { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
            inputAssembly.topology = T::topology;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

            VkPipelineViewportStateCreateInfo viewportState
            { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            VkPipelineDepthStencilStateCreateInfo depthStencil = depthStencilState();

            VkGraphicsPipelineCreateInfo pipelineInfo
            { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
            pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.layout = layout;

            pipelineInfo.renderPass = SwapChain::renderPass;
            pipelineInfo.subpass = 0;

            pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineInfo.pStages = shaderStages.data();

            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pDynamicState = dynamicState(dynamicStates);
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = rasterState(VK_POLYGON_MODE_FILL);;
            pipelineInfo.pMultisampleState = msaaState(VK_TRUE, 0.2f);
            pipelineInfo.pColorBlendState = colorBlendState(VK_FALSE);
            pipelineInfo.pDepthStencilState = &depthStencil;

            if (vkCreateGraphicsPipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }
        }
    };

    struct derivativePPL : Pipeline  {
        derivativePPL(VkPipeline& parentPPL, std::set<Shader>& shaders, std::vector<VkDescriptorSetLayout>& setLayouts)
        {
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            vkLoadSetLayout(setLayouts);
            vkCreatePipeline(parentPPL, shaders);
        }
        ~derivativePPL() {
            std::jthread t0([&] { vkDestroyPipeline(GPU::device, pipeline, nullptr); });
            std::jthread t1([&] { vkDestroyPipelineLayout(GPU::device, layout, nullptr); });
        }
    protected:
        void vkCreatePipeline(VkPipeline& parentPPL, std::set<Shader>& shaders) {
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
            for (Shader shader : shaders) {
                shaderStages.push_back(shader.stageInfo);
            }
            VkGraphicsPipelineCreateInfo pipelineInfo
            { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
            pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
            pipelineInfo.basePipelineHandle = parentPPL;
            pipelineInfo.layout = layout;

            pipelineInfo.renderPass = SwapChain::renderPass;
            pipelineInfo.subpass = 0;

            pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineInfo.pStages = shaderStages.data();

            pipelineInfo.pRasterizationState = &rasterState(VK_POLYGON_MODE_LINE);;
            pipelineInfo.pMultisampleState = &msaaState(VK_TRUE, 0.2f);

            if (vkCreateGraphicsPipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }
        }
    };
}