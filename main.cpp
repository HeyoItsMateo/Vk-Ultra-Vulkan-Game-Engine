#include "VulkanEngine.h"

#include <algorithm>
#include <functional>

VkWindow window("Vulkan");
VkGraphicsEngine app;

UBO uniforms;

pop population;

Octree tree(vertices, 0.01f);

SSBO<Particle> ssbo(population.particles, VK_SHADER_STAGE_COMPUTE_BIT);
SSBO<glm::mat4> modelSSBO(tree.matrices, VK_SHADER_STAGE_VERTEX_BIT);

VkUniformBuffer ubo(uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);


VkTexture planks("textures/planks.png");
VkTextureSet textureSet(planks);

VkDescriptorSet sets[] = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };
VkDescriptorSet sets2[] = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], modelSSBO.Sets[VkSwapChain::currentFrame] };

std::vector<VkDescriptorSet> segs = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], ssbo.Sets[VkSwapChain::currentFrame] };
//std::vector<VkDescriptorSet> segs2 = { ubo.Sets[VkSwapChain::currentFrame], textureSet.Sets[VkSwapChain::currentFrame], modelSSBO.Sets[VkSwapChain::currentFrame] };

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



VkGraphicsPipeline<Vertex> pipeline(descriptors, segs, shaders);

VkGraphicsPipeline<Octree> octreePipeline(descriptors, segs, octreeShaders);

VkTestPipeline<Particle> ptclPipeline(descriptors, segs, compShaders);

VkComputePipeline testPpln(descriptors, segs, compShader.stageInfo);

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
        app.run(pipeline, ptclPipeline, octreePipeline, tree, testPpln, ssbo, uniforms, ubo);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}