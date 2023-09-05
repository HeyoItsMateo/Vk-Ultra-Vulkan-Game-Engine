//#include "vk.gpu.h"
#include "vk.swapchain.h"
#include "vk.cpu.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <execution>

#include <variant>

#include "Primitives.h"
#include "Buffers.h"

#include "Camera.h"
#include "Model.h"
#include "Textures.h" //Used in descriptors
#include "Octree.h"

#include "file_system.h" // Used for the shaders
#include "helperFunc.h" // Used for descriptor packing

#include "descriptors.h"

//typedef void(__stdcall* vkDestroyFunction)(VkDevice, void ,const VkAllocationCallbacks*);

namespace vk {
    struct Shader {
        VkShaderModule shaderModule;
        VkPipelineShaderStageCreateInfo stageInfo
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        Shader(const std::string& filename, VkShaderStageFlagBits shaderStage) {
            try {
                std::filesystem::path filepath(filename);
                checkLog(filename);

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

    struct ShaderSet {
        std::vector<VkShaderModule> shaderModules;
        std::vector<VkPipelineShaderStageCreateInfo> setInfo;
        ShaderSet(const std::vector<std::string>& filenames) {
            shaderModules.resize(filenames.size());
            std::vector<std::vector<char>> shaderCodes;
            for (auto& filename : filenames) {
                shaderCodes.push_back(readFile(".\\shaders\\" + filename + ".spv"));
            }
            createShaderModule(shaderCodes);
        }
    private:
        void createShaderModule(const std::vector<std::vector<char>>& Code) {
            std::vector<VkShaderModuleCreateInfo> shaderInfo;
            for (auto& code : Code) {
                VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
                info.codeSize = code.size();
                info.pCode = reinterpret_cast<const uint32_t*>(code.data());
                shaderInfo.push_back(info);
            }
            if (vkCreateShaderModule(GPU::device, shaderInfo.data(), nullptr, shaderModules.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader modules!");
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


    struct Pipeline {
    public:
        VkPipeline pipeline;
        VkPipelineLayout layout;
        std::vector<VkDescriptorSet> sets;
        void bind() {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];

            vkCmdBindDescriptorSets(commandBuffer, bindPoint, layout, 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
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

    template<typename T>
    struct GraphicsPPL : Pipeline {
        GraphicsPPL(std::vector<Shader*>& shaders, std::vector<VkDescriptorSet>& descSets, std::vector<VkDescriptorSetLayout>& setLayouts)
        {//TODO: Rewrite 'VkCreatePipeline' to change based on needed vertex input,
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sets = descSets;
            vkLoadSetLayout(setLayouts);
            vkCreatePipeline(shaders);
        }
        ~GraphicsPPL() {
            std::jthread t0([&] { vkDestroyPipeline(GPU::device, pipeline, nullptr); });
            std::jthread t1([&] { vkDestroyPipelineLayout(GPU::device, layout, nullptr); });
        }
    protected:
        void vkCreatePipeline(std::vector<Shader*>& shaders) {
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages = packMembers<&Shader::stageInfo>(shaders);
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = T::vkCreateVertexInput();

            VkPipelineInputAssemblyStateCreateInfo inputAssembly
            { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
            inputAssembly.topology = T::topology;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicStateInfo = dynamicState(dynamicStates);

            VkPipelineViewportStateCreateInfo viewportState
            { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineRasterizationStateCreateInfo rasterInfo = rasterState(VK_POLYGON_MODE_LINE);            
            VkPipelineMultisampleStateCreateInfo msaaInfo = msaaState(VK_TRUE, 0.2f);
            VkPipelineColorBlendStateCreateInfo colorBlendInfo = colorBlendState(colorBlendAttachment, VK_FALSE);
            VkPipelineDepthStencilStateCreateInfo depthStencilInfo = depthStencilState();

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

    struct derivativePPL : Pipeline {
        derivativePPL(VkPipeline& parentPPL, std::vector<Shader*>& shaders, std::vector<VkDescriptorSet>& descSets, std::vector<VkDescriptorSetLayout>& setLayouts)
        {
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sets = descSets;
            vkLoadSetLayout(setLayouts);
            vkCreatePipeline(parentPPL, shaders);
        }
        ~derivativePPL() {
            std::jthread t0([&] { vkDestroyPipeline(GPU::device, pipeline, nullptr); });
            std::jthread t1([&] { vkDestroyPipelineLayout(GPU::device, layout, nullptr); });
        }
    protected:
        void vkCreatePipeline(VkPipeline& parentPPL, std::vector<Shader*>& shaders) {
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages = packMembers<&Shader::stageInfo>(shaders);
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = Voxel::vkCreateVertexInput();

            VkPipelineInputAssemblyStateCreateInfo inputAssembly
            { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
            inputAssembly.topology = Voxel::topology;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            VkPipelineDynamicStateCreateInfo dynamicStateInfo = dynamicState(dynamicStates);

            VkPipelineViewportStateCreateInfo viewportState
            { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineRasterizationStateCreateInfo rasterInfo = rasterState(VK_POLYGON_MODE_LINE);
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

    template<VkPipelineBindPoint bindPoint>
    struct PipelineBase {
        VkPipeline mPipeline;
        VkPipelineLayout mLayout;
        PipelineBase(std::vector<VkDescriptorSet>& Sets, std::vector<VkDescriptorSetLayout>& Layouts) {
            setCount = static_cast<uint32_t>(Sets.size());
            sets = Sets;

            vkLoadSetLayout(Layouts);
        }
        ~PipelineBase() {
            std::jthread t0(vkDestroyPipeline, GPU::device, std::ref(mPipeline), nullptr);
            std::jthread t1(vkDestroyPipelineLayout, GPU::device, std::ref(mLayout), nullptr);
        }
        void bind() {
            VkCommandBuffer& commandBuffer = EngineCPU::renderCommands[SwapChain::currentFrame];

            vkCmdBindDescriptorSets(commandBuffer, bindPoint, mLayout, 0, setCount, sets.data(), 0, nullptr);
            vkCmdBindPipeline(commandBuffer, bindPoint, mPipeline);
        }
    protected:
        uint32_t setCount;
        std::vector<VkDescriptorSet> sets;
        VkPipelineBindPoint bindPoint = bindPoint;
        void vkLoadSetLayout(std::vector<VkDescriptorSetLayout>& SetLayout) {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo
            { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(SetLayout.size());
            pipelineLayoutInfo.pSetLayouts = SetLayout.data();

            if (vkCreatePipelineLayout(GPU::device, &pipelineLayoutInfo, nullptr, &mLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }
        }
    };

    struct ParticlePipeline : Pipeline {
        ParticlePipeline(VkPipeline& parentPPL, std::vector<Shader*>& shaders, std::vector<VkDescriptorSet>& descSets, std::vector<VkDescriptorSetLayout>& setLayouts)
        {//TODO: Rewrite 'VkCreatePipeline' to change based on needed vertex input,
            bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sets = descSets;
            vkLoadSetLayout(setLayouts);
            vkCreatePipeline(parentPPL, shaders);
        }
    private:
        void vkCreatePipeline(VkPipeline& parentPPL, std::vector<Shader*>& shaders) {
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages = packMembers<&Shader::stageInfo>(shaders);
            //auto bindingDescription = T::vkCreateBindings();
            //auto attributeDescriptions = T::vkCreateAttributes();
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = Particle::vkCreateVertexInput();

            VkPipelineInputAssemblyStateCreateInfo inputAssembly
            { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkPipelineRasterizationStateCreateInfo rasterizer
            { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };

            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_POINT;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;

            VkPipelineViewportStateCreateInfo viewportState
            { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
            viewportState.viewportCount = 1;
            viewportState.scissorCount = 1;

            //VkPipelineRasterizationStateCreateInfo rasterizer = Utilities::vkCreateRaster(VK_POLYGON_MODE_FILL);

            VkPipelineMultisampleStateCreateInfo multisampling
            { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
            multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
            multisampling.rasterizationSamples = GPU::msaaSamples;

            VkPipelineDepthStencilStateCreateInfo depthStencil = Utilities::vkCreateDepthStencil();

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo colorBlending = Utilities::vkCreateColorBlend(colorBlendAttachment, VK_FALSE);

            std::vector<VkDynamicState> dynamicStates = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };
            VkPipelineDynamicStateCreateInfo dynamicState
            { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
            dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
            dynamicState.pDynamicStates = dynamicStates.data();

            VkGraphicsPipelineCreateInfo pipelineInfo
            { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
            pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
            pipelineInfo.basePipelineHandle = parentPPL;
            pipelineInfo.basePipelineIndex = -1;
            pipelineInfo.renderPass = SwapChain::renderPass;
            pipelineInfo.subpass = 0;

            pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineInfo.pStages = shaderStages.data();
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.layout = layout;

            if (vkCreateGraphicsPipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }
        }
    };

    struct ComputePipeline : PipelineBase<VK_PIPELINE_BIND_POINT_COMPUTE> {
        const uint32_t PARTICLE_COUNT = 1000;
        ComputePipeline(std::vector<VkDescriptorSet>& sets, VkPipelineShaderStageCreateInfo& computeStage, std::vector<VkDescriptorSetLayout>& testing)
            : PipelineBase(sets, testing)
        {
            vkCreatePipeline(computeStage);
        }

        void run() {
            VkCommandBuffer& commandBuffer = EngineCPU::computeCommands[SwapChain::currentFrame];

            VkCommandBufferBeginInfo beginInfo
            { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mLayout, 0, setCount, sets.data(), 0, nullptr);

            vkCmdDispatch(commandBuffer, PARTICLE_COUNT / (100), PARTICLE_COUNT / (100), PARTICLE_COUNT / (100));

            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to record compute command buffer!");
            }
        }
    private:
        void vkCreatePipeline(VkPipelineShaderStageCreateInfo& computeStage) {
            VkComputePipelineCreateInfo pipelineInfo
            { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
            pipelineInfo.layout = mLayout;
            pipelineInfo.stage = computeStage;

            if (vkCreateComputePipelines(GPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create compute pipeline!");
            }
        }
    };
}



