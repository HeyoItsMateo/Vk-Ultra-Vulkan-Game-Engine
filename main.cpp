#include "VulkanEngine.h"

#include <algorithm>
#include <functional>

VkWindow window("Vulkan");
VkGraphicsEngine app;

Uniforms uniforms;
UBO ubo(uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);




VkTexture planks("textures/planks.png");
//std::vector<VkTexture> textures{planks};
VkTextureSet textureSet(planks);

pop population;
SSBO ssbo(population.particles, VK_SHADER_STAGE_COMPUTE_BIT);

Octree tree(vertices, 0.01f);
SSBO modelSSBO(tree.matrices, VK_SHADER_STAGE_VERTEX_BIT);

PhxModel model(vertices, indices);

std::vector<VkDescriptorSet> segs = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };
//std::vector<VkDescriptorSet> segs2 = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], modelSSBO.Sets[VkSwapChain::currentFrame] };
std::vector<VkDescriptorSetLayout> testing = { ubo.SetLayout, textureSet.SetLayout, ssbo.SetLayout };

std::vector<VkDescriptor*> descriptors = { &ubo, &textureSet, &ssbo };
std::vector<VkDescriptor*> desc2 = { &ubo, &textureSet, &modelSSBO };

VkShader vertShader("shaders/vertexVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader fragShader("shaders/vertexFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
VkShader compShader("shaders/pointComp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
std::vector<VkShader*> shaders = { &vertShader, &fragShader };

VkShader pointVert("shaders/pointVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader pointFrag("shaders/pointFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
std::vector<VkShader*> compShaders = { &pointVert, &pointFrag };

VkShader octreeVert("shaders/octreeVert.spv", VK_SHADER_STAGE_VERTEX_BIT);
VkShader octreeFrag("shaders/octreeFrag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
std::vector<VkShader*> octreeShaders = { &octreeVert, &octreeFrag };



VkGraphicsPipeline<Vertex> pipeline(descriptors, segs, shaders, testing);

VkGraphicsPipeline<Cube> octreePPL(descriptors, segs, octreeShaders, testing);

VkParticlePipeline particlePPL(descriptors, segs, compShaders, testing);

VkComputePipeline computePPL(descriptors, segs, compShader.stageInfo, testing);

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
        app.run(pipeline, octreePPL, model, tree, particlePPL, computePPL, ssbo, ubo, uniforms);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}