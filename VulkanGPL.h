#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <execution>

#include <variant>
#include <chrono>
#include <random>

#include "VulkanGPU.h"
#include "VulkanCPU.h"

#include "PhysX.h"

#include "Camera.h"
#include "Model.h"

#include "UBO.h"
#include "SSBO.h"

#include "helperFunc.h"
#include "Octree.h"

#include "descriptors.h"
#include "bufferObjects.h"
//typedef void(__stdcall* vkDestroyFunction)(VkDevice, void ,const VkAllocationCallbacks*);

std::vector<Vertex> vertices = {
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

typedef Primitive<VK_PRIMITIVE_TOPOLOGY_POINT_LIST> Point;
//typedef Primitive<VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST> Triangle;
/*
struct Particle : Point {
    glm::vec3 velocity;
};
*/

struct VkShader {
    VkShaderModule shaderModule;
    VkPipelineShaderStageCreateInfo stageInfo
    { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    VkShader(const std::string& filename, VkShaderStageFlagBits shaderStage) {
        auto shaderCode = readFile(filename);
        createShaderModule(shaderCode);

        stageInfo.stage = shaderStage;
        stageInfo.module = shaderModule;
        stageInfo.pName = "main";
    }
    ~VkShader() {
        vkDestroyShaderModule(VkGPU::device, shaderModule, nullptr);
    }
private:
    void createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo
        { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        if (vkCreateShaderModule(VkGPU::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create" + std::string(typeid(shaderModule).name()) + "!");
        }
    }
    // File Reader
    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
};

template<VkPipelineBindPoint bindPoint>
struct VkPipelineBase {
    VkPipeline mPipeline;
    VkPipelineLayout mLayout;
    VkPipelineBase(std::vector<VkDescriptor*>& descriptors, std::vector<VkDescriptorSet>& Sets, std::vector<VkDescriptorSetLayout>& testing) {
        setCount = Sets.size();
        sets = Sets;
        std::vector<VkDescriptorSetLayout> layout = packMembers<&VkDescriptor::SetLayout>(descriptors);
        vkLoadSetLayout(testing);
    }
    ~VkPipelineBase() {
        std::jthread t0(vkDestroyPipeline, VkGPU::device, std::ref(mPipeline), nullptr);
        std::jthread t1(vkDestroyPipelineLayout, VkGPU::device, std::ref(mLayout), nullptr);
    }
    void bind() {
        VkCommandBuffer& commandBuffer = VkEngineCPU::renderCommands[VkSwapChain::currentFrame];

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

        if (vkCreatePipelineLayout(VkGPU::device, &pipelineLayoutInfo, nullptr, &mLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }    
};

typedef VkPipelineBase<VK_PIPELINE_BIND_POINT_GRAPHICS> GraphicsPipeline;

template<typename T>
struct VkGraphicsPipeline : GraphicsPipeline {
    VkGraphicsPipeline(std::vector<VkDescriptor*>& descriptors, std::vector<VkDescriptorSet>& sets, std::vector<VkShader*>& shaders, std::vector<VkDescriptorSetLayout>& testing)
        : GraphicsPipeline(descriptors, sets, testing)
    {
        //bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        //TODO: Rewrite 'VkCreatePipeline' to change based on needed vertex input,
        //      and seperate the member variable 'model' from the pipeline
        vkCreatePipeline(shaders);
    }
private:
    void vkCreatePipeline(std::vector<VkShader*>& shaders) {
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = packMembers<&VkShader::stageInfo>(shaders);
        //auto bindingDescription = T::vkCreateBindings();
        //auto attributeDescriptions = T::vkCreateAttributes();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = Vertex::vkCreateVertexInput();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly
        { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = T::topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState
        { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer = VkUtils::vkCreateRaster(VK_POLYGON_MODE_FILL);

        VkPipelineMultisampleStateCreateInfo multisampling
        { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
        multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
        multisampling.rasterizationSamples = VkGPU::msaaSamples;

        VkPipelineDepthStencilStateCreateInfo depthStencil = VkUtils::vkCreateDepthStencil();

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = VkUtils::vkCreateColorBlend(colorBlendAttachment, VK_FALSE);

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
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = mLayout;
        pipelineInfo.renderPass = VkSwapChain::renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.pDepthStencilState = &depthStencil;

        if (vkCreateGraphicsPipelines(VkGPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }
};

struct VkParticlePipeline : GraphicsPipeline {
    VkParticlePipeline(std::vector<VkDescriptor*>& descriptors, std::vector<VkDescriptorSet>& sets, std::vector<VkShader*>& shaders, std::vector<VkDescriptorSetLayout>& testing)
        : GraphicsPipeline(descriptors, sets, testing)
    {
        //bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        //TODO: Rewrite 'VkCreatePipeline' to change based on needed vertex input,
        //      and seperate the member variable 'model' from the pipeline
        vkCreatePipeline(shaders);
    }    
    void draw(VkBuffer& buffer) {
        VkCommandBuffer& commandBuffer = VkEngineCPU::renderCommands[VkSwapChain::currentFrame];

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &buffer, offsets); // &SSBO.mBuffer
        vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);
    }
private:
    void vkCreatePipeline(std::vector<VkShader*>& shaders) {
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = packMembers<&VkShader::stageInfo>(shaders);
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
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState
        { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        //VkPipelineRasterizationStateCreateInfo rasterizer = VkUtils::vkCreateRaster(VK_POLYGON_MODE_FILL);

        VkPipelineMultisampleStateCreateInfo multisampling
        { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.sampleShadingEnable = VK_TRUE; // enable sample shading in the pipeline
        multisampling.minSampleShading = .2f; // min fraction for sample shading; closer to one is smoother
        multisampling.rasterizationSamples = VkGPU::msaaSamples;

        VkPipelineDepthStencilStateCreateInfo depthStencil = VkUtils::vkCreateDepthStencil();

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = VkUtils::vkCreateColorBlend(colorBlendAttachment, VK_FALSE);

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
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = mLayout;
        pipelineInfo.renderPass = VkSwapChain::renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.pDepthStencilState = &depthStencil;

        if (vkCreateGraphicsPipelines(VkGPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }
};

struct VkComputePipeline : VkPipelineBase<VK_PIPELINE_BIND_POINT_COMPUTE> {
    VkComputePipeline(std::vector<VkDescriptor*>& descriptors, std::vector<VkDescriptorSet>& sets, VkPipelineShaderStageCreateInfo& computeStage, std::vector<VkDescriptorSetLayout>& testing)
        : VkPipelineBase(descriptors, sets, testing)
    {
        vkCreatePipeline(computeStage);
    }

    void run() {
        VkCommandBuffer& commandBuffer = VkEngineCPU::computeCommands[VkSwapChain::currentFrame];

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

        if (vkCreateComputePipelines(VkGPU::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }
    }
};

