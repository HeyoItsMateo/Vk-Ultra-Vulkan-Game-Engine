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

std::vector<VkDescriptorSet> descSet1 = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };
std::vector<VkDescriptorSetLayout> testing = { ubo.SetLayout, textureSet.SetLayout, ssbo.SetLayout };

std::vector<VkDescriptorSet> descSet2 = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], modelSSBO.Sets[VkSwapChain::currentFrame] };
std::vector<VkDescriptorSetLayout> testing2 = { ubo.SetLayout, textureSet.SetLayout, modelSSBO.SetLayout };

VkShader vertShader("vertex.vert", VK_SHADER_STAGE_VERTEX_BIT);
VkShader fragShader("vertex.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
VkShader compShader("shader.comp", VK_SHADER_STAGE_COMPUTE_BIT);
std::vector<VkShader*> shaders = { &vertShader, &fragShader };

VkShader pointVert("point.vert", VK_SHADER_STAGE_VERTEX_BIT);
VkShader pointFrag("point.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
std::vector<VkShader*> compShaders = { &pointVert, &pointFrag };

VkShader octreeVert("octree.vert", VK_SHADER_STAGE_VERTEX_BIT);
VkShader octreeFrag("octree.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
std::vector<VkShader*> octreeShaders = { &octreeVert, &octreeFrag };

VkGraphicsPipeline<Vertex> pipeline(descSet1, shaders, testing);

VkGraphicsPipeline<Voxel> octreePPL(descSet2, octreeShaders, testing2);

VkParticlePipeline particlePPL(descSet1, compShaders, testing);

VkComputePipeline computePPL(descSet1, compShader.stageInfo, testing);


//TODO: Learn discrete differential geometry
//TODO: Implement waves~
//TODO: Optimize the swapchain and rendering process
//TODO: Optimize a fuckload of stuff with shader caching, pipeline caching, parallelization, etc.

VkPhysicalDeviceProperties properties;

int main() {
    

    vkGetPhysicalDeviceProperties(VkGPU::physicalDevice, &properties);

    std::printf("maxWorkgroupSize is: %i \n", *properties.limits.maxComputeWorkGroupSize);
    std::printf("maxWorkGroupInvocations is: %i \n", properties.limits.maxComputeWorkGroupInvocations);
    std::printf("maxWorkGroupCount is: %i \n", *properties.limits.maxComputeWorkGroupCount);

    try {
        app.run(pipeline, octreePPL, model, tree, particlePPL, computePPL, ssbo, ubo, uniforms);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
