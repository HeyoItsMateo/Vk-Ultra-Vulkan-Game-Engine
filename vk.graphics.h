//#include "vk.gpu.h"
#pragma once

#include "vk.swapchain.h"
#include "vk.cpu.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <string>
#include <execution>

#include <variant>

#include "Primitives.h"
#include "vk.buffers.h"

#include "Camera.h"
#include "Model.h"
#include "Textures.h" //Used in descriptors
#include "Octree.h"

#include "file_system.h" // Used for the shaders
#include "helperFunc.h" // Used for descriptor packing

#include "descriptors.h"

//#define VK_DESTRUCTOR(object) VKAPI_ATTR void VKAPI_CALL vkDestroy##object(VkDevice device, Vk##object object, const VkAllocationCallbacks* pAllocator);
//VK_DESTRUCTOR(ShaderModule)


namespace vk {    
    struct Shader {
        VkShaderModule shaderModule;
        VkShaderStageFlagBits shaderStage;
        Shader(std::string const& filename, VkShaderStageFlagBits stage)
            : shaderStage(stage)
        {
            checkLog(filename);
            auto code = readFile(".\\shaders\\" + filename + ".spv");
            createShaderModule(code, filename);
        }
        ~Shader() {
            if (shaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule(GPU::device, shaderModule, nullptr);
            }
        }
        //TODO: Create copy and move constructors
    private:
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
        void createShaderModule(const std::vector<char>& code, const std::string& filename) {
            VkShaderModuleCreateInfo createInfo
            { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            if (vkCreateShaderModule(GPU::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create " + filename + "!");
            }
        }
    };

    struct Pipeline {
    public:
        VkPipeline pipeline{};
        VkPipelineLayout layout{};
        std::vector<VkDescriptorSet> sets;
        virtual void bind() {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];

            vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
            vkCmdBindPipeline(commandBuffer, bindPoint, pipeline);
        }
        ~Pipeline() {
            std::jthread t0([&] { vkDestroyPipeline(GPU::device, pipeline, nullptr); });
            std::jthread t1([&] { vkDestroyPipelineLayout(GPU::device, layout, nullptr); });
        }
    protected:
        VkPipelineBindPoint bindPoint{};
        void vkLoadSetLayout(std::vector<VkDescriptorSetLayout>& SetLayout) {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo
            { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(SetLayout.size());
            pipelineLayoutInfo.pSetLayouts = SetLayout.data();

            if (vkCreatePipelineLayout(GPU::device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
        }
        virtual std::vector<VkPipelineShaderStageCreateInfo> stageInfo(std::vector<Shader>& shaders) {
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

        static VkPipelineInputAssemblyStateCreateInfo inputAssembly(VkPrimitiveTopology topology) {
            VkPipelineInputAssemblyStateCreateInfo inputAssembly
            { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
            inputAssembly.topology = topology;
            inputAssembly.primitiveRestartEnable = VK_FALSE;
            return inputAssembly;
        }
        static VkPipelineViewportStateCreateInfo viewportState(uint32_t viewportCount, uint32_t scissorCount, VkViewport* pViewports = nullptr, VkRect2D* pScissors = nullptr) {
            VkPipelineViewportStateCreateInfo viewport_state
            { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
            viewport_state.viewportCount = viewportCount;
            viewport_state.scissorCount = scissorCount;
            viewport_state.pViewports = pViewports;
            viewport_state.pScissors = pScissors;

            return viewport_state;
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
        static VkPipelineColorBlendStateCreateInfo colorBlendState(VkPipelineColorBlendAttachmentState& colorBlendAttachment,VkBool32 logicOpEnable, VkLogicOp logicOp = VK_LOGIC_OP_COPY) {
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

    template<typename primitiveType, VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL>
    struct GraphicsPPL : Pipeline {
        template<uint32_t size>
        GraphicsPPL(Shader (&shaders)[size], std::vector<VkDescriptorSet> const& descSets, std::vector<VkDescriptorSetLayout>& SetLayout)
        {
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sets = descSets;
            vkLoadSetLayout(SetLayout);
            vkCreatePipeline(stageInfo(shaders, size));
        }

    private:
        std::vector<VkPipelineShaderStageCreateInfo> stageInfo(Shader* shaders, uint32_t size) {
            VkPipelineShaderStageCreateInfo stageInfo{};
            stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageInfo.pName = "main";

            std::vector<VkPipelineShaderStageCreateInfo> shader_stages{size, stageInfo};
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

            if (vkCreateGraphicsPipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }
        }
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

            if (vkCreateGraphicsPipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }
        }
    };

    struct ComputePPL : Pipeline {
        const uint32_t PARTICLE_COUNT = 1000;
        ComputePPL(Shader const& computeShader, std::vector<VkDescriptorSet>& descSets, std::vector<VkDescriptorSetLayout>& setLayouts)
        {
            bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
            sets = descSets;
            vkLoadSetLayout(setLayouts);
            vkCreatePipeline(computeShader);
        }
        void run() {
            VkCommandBuffer& commandBuffer = EngineCPU::computeCommands[SwapChain::currentFrame];
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);

            vkCmdDispatch(commandBuffer, PARTICLE_COUNT / (100), PARTICLE_COUNT / (100), PARTICLE_COUNT / (100));

            
        }
    private:
        void vkCreatePipeline(Shader const& computeShader) {
            VkPipelineShaderStageCreateInfo stageInfo
            { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            stageInfo.module = computeShader.shaderModule;
            stageInfo.stage = computeShader.shaderStage;
            stageInfo.pName = "main";

            VkComputePipelineCreateInfo pipelineInfo
            { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
            pipelineInfo.layout = layout;
            pipelineInfo.stage = stageInfo;

            VK_CHECK_RESULT(vkCreateComputePipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));
        }
    };
}



