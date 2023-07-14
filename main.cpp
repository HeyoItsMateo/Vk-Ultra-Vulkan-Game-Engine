#include "VulkanEngine.h"

#include <algorithm>
#include <functional>

VkWindow window("Vulkan");
VkGraphicsEngine app;

UBO uniforms;
SSBO shaderStorage;

VkUniformBuffer<UBO> ubo(uniforms, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
VkStorageBuffer ssbo(shaderStorage, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

VkTexture planks("textures/planks.png");
VkTextureSet textureSet(planks);

VkDescriptorSet sets[] = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };

std::vector<VkDescriptor*> descriptors = { &ubo, &textureSet, &ssbo };

VkShader vertShader("shaders/vertexVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader fragShader("shaders/vertexFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
VkShader compShader("shaders/pointComp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

VkShader pointVert("shaders/pointVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader pointFrag("shaders/pointFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

std::vector<VkShader*> shaders = { &vertShader, &fragShader };
std::vector<VkShader*> compShaders = { &pointVert, &pointFrag};

VkGraphicsPipeline<Vertex> pipeline(descriptors, shaders);

VkTestPipeline<Particle> ptclPipeline(descriptors, compShaders);

VkComputePipeline testPpln(descriptors, compShader.stageInfo);

//TODO: Learn discrete differential geometry
//TODO: Implement waves~
//TODO: Optimize the swapchain and rendering process
//TODO: Optimize a fuckload of stuff with shader caching, pipeline caching, parallelization, etc.

void printMsg(const char* message, size_t value) {
    std::cout << message << " " << value << std::endl;
};

VkPhysicalDeviceProperties properties;


int main() {
    vkGetPhysicalDeviceProperties(VkGPU::physicalDevice, &properties);

    printMsg("maxWorkgroupSize is:", *properties.limits.maxComputeWorkGroupSize);
    printMsg("maxWorkGroupInvocations is:", properties.limits.maxComputeWorkGroupInvocations);
    printMsg("maxWorkGroupCount is:", *properties.limits.maxComputeWorkGroupCount);

    try {
        app.run(pipeline, ptclPipeline, testPpln, ssbo, uniforms, ubo, 3, sets);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}