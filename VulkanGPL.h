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
#include "Camera.h"
#include "Model.h"
#include "DBO.h"

const std::vector<Vertex> vertices = {
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

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 color;

    static VkVertexInputBindingDescription vkCreateBindings() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> vkCreateAttributes() {
        std::vector<VkVertexInputAttributeDescription> Attributes{
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Particle, position)},
            {1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, color)}
        };
        return Attributes;
    }
};

struct UBO {
    modelMatrix model;
    camMatrix camera;
    float dt;
    void update(GLFWwindow* window, VkExtent2D extent, float FOVdeg = 45.f, float nearPlane = 0.1f, float farPlane = 10.f) {
        std::jthread t1( [this] 
            { model.update(); }
        );
        std::jthread t2( [this, window, extent, FOVdeg, nearPlane, farPlane] 
            { camera.update(window, extent, FOVdeg, nearPlane, farPlane); }
        );
    }
};

struct SSBO {
    std::vector<Particle> particles{1000};
    SSBO(int width, int height) {
        populate(particles, width, height);
    }
private:
    void populate(std::vector<Particle>& particles, int width, int height) {
        std::default_random_engine rndEngine((unsigned)time(nullptr));
        std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

        std::for_each(std::execution::par,
            particles.begin(), particles.end(),
            [&](Particle particle)
            {// Populate particle system
                float r = 0.25f * sqrt(rndDist(rndEngine));
                float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
                float x = r * cos(theta) * height / width;
                float y = r * sin(theta);
                float z = r * cos(theta) / sin(theta);
                particle.position = glm::vec3(x, y, z);
                particle.velocity = glm::normalize(glm::vec3(x, y, z)) * 0.00025f;
                particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
            });

    }
};

struct VkShader {
    VkShaderModule shaderModule;
    VkPipelineShaderStageCreateInfo stageInfo{};
    VkShader(const std::string& filename, VkShaderStageFlagBits shaderStage) {
        auto shaderCode = readFile(filename);
        createShaderModule(shaderCode);

        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = shaderStage;
        stageInfo.module = shaderModule;
        stageInfo.pName = "main";
    }
    ~VkShader() {
        vkDestroyShaderModule(VkGPU::device, shaderModule, nullptr);
    }
private:
    void createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        if (vkCreateShaderModule(VkGPU::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
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


struct VkGraphicsPipeline {
    VkPipeline mPipeline;
    VkPipelineLayout mLayout;

    VkBufferObject<Vertex> VBO;
    VkBufferObject<uint16_t> EBO;
    
    UBO uniforms;
    VkDataBuffer<UBO> ubo;

    SSBO shaderStorage;
    VkDataBuffer<SSBO> ssbo;
    
    VkTexture planks; 
    VkTextureSet textureSet;

    VkGraphicsPipeline(VkSwapChain& swapChain) :
        VBO(vertices), EBO(indices),

        ubo(uniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),

        shaderStorage(swapChain.Extent.height,swapChain.Extent.width),
        ssbo(shaderStorage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT),

        planks("textures/planks.png"),
        textureSet(planks) {
        createShaderPipeline<Vertex>();
    }
    ~VkGraphicsPipeline() {
        vkDestroyPipeline(VkGPU::device, mPipeline, nullptr);
        vkDestroyPipelineLayout(VkGPU::device, mLayout, nullptr);
    }
private:
    template<typename T>
    void createShaderPipeline() {
        VkDescriptorSetLayout layouts[] = { ubo.SetLayout, textureSet.SetLayout, ssbo.SetLayout };
        vkLoadSetLayout(layouts, sizeof(layouts)/sizeof(VkDescriptorSetLayout));

        VkShader vertShader("shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        VkShader fragShader("shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
        VkShader compShader("shaders/comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShader.stageInfo, fragShader.stageInfo, compShader.stageInfo };

        auto bindingDescription = T::vkCreateBindings();
        auto attributeDescriptions = T::vkCreateAttributes();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = VkUtils::vkCreateVertexInput(bindingDescription, attributeDescriptions);

        VkPipelineInputAssemblyStateCreateInfo inputAssembly
        { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
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
    void vkLoadSetLayout(VkDescriptorSetLayout* SetLayout, uint32_t LayoutCount) {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo
        { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutInfo.setLayoutCount = LayoutCount;
        pipelineLayoutInfo.pSetLayouts = SetLayout;

        if (vkCreatePipelineLayout(VkGPU::device, &pipelineLayoutInfo, nullptr, &mLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }
    }
};