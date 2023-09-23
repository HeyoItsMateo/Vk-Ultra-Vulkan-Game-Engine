#include "vk.engine.h"
#include "Textures.h"
#include "Octree.h"

#include <algorithm>
#include <functional>

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

vk::Window window("Vulkan");
vk::Engine app;

vk::Uniforms uniforms;
vk::UBO ubo(uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

pop population(1000);
vk::SSBO ssbo(population.particles, VK_SHADER_STAGE_COMPUTE_BIT);

vk::CombinedImageSampler planksImageSampler("textures/planks.png");

vk::Texture planks("textures/planks.png");
vk::Sampler sampler(planks.mipLevels);

std::vector<triangleList> vertices = {
    {{-0.5f,  0.5f,  0.5f, 1.f}, {-0.5f,  0.5f,  0.5f, 1.f}, {1.0f, 0.0f, 0.0f, 1.f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f, 1.f}, { 0.5f,  0.5f,  0.5f, 1.f}, {0.0f, 1.0f, 0.0f, 1.f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f, 1.f}, { 0.5f,  0.5f, -0.5f, 1.f}, {0.0f, 0.0f, 1.0f, 1.f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f, 1.f}, {-0.5f,  0.5f, -0.5f, 1.f}, {1.0f, 1.0f, 1.0f, 1.f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f,  0.5f, 1.f}, {-0.5f, -0.5f,  0.5f, 1.f}, {1.0f, 0.0f, 0.0f, 1.f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f, 1.f}, { 0.5f, -0.5f,  0.5f, 1.f}, {0.0f, 1.0f, 0.0f, 1.f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f, 1.f}, { 0.5f, -0.5f, -0.5f, 1.f}, {0.0f, 0.0f, 1.0f, 1.f}, {1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f, 1.f}, {-0.5f, -0.5f, -0.5f, 1.f}, {1.0f, 1.0f, 1.0f, 1.f}, {0.0f, 1.0f}}
};
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
vk::Mesh model(vertices, indices);

std::vector<VkDescriptorSet> plankSet{
    ubo.Sets[vk::SwapChain::currentFrame],
    sampler.Sets[vk::SwapChain::currentFrame],
    planks.Sets[vk::SwapChain::currentFrame]
};
std::vector<VkDescriptorSetLayout> plankLayout{
    ubo.SetLayout,
    sampler.SetLayout,
    planks.SetLayout
};

//vk::Descriptor descriptors[] = {
//    vk::UBO{ uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT },
//    vk::Sampler{ planks.mipLevels },
//    vk::Texture{ "textures/planks.png" }
//};

vk::Shader plankShaders[] = {
    {"vertex.vert", VK_SHADER_STAGE_VERTEX_BIT},
    {"vertex.frag", VK_SHADER_STAGE_FRAGMENT_BIT}
};
vk::GraphicsPPL<triangleList> graphicsPPL(plankShaders, plankSet, plankLayout);





vk::Plane plane({ 300, 200 }, { 0.025, 0.025 });

vk::UBO modMat(plane.matrix, VK_SHADER_STAGE_VERTEX_BIT);
vk::Sampler topographySampler(VK_SHADER_STAGE_VERTEX_BIT);
vk::ComputeImage heightMap;

std::vector<VkDescriptorSet> planeSet{
    ubo.Sets[vk::SwapChain::currentFrame],
    modMat.Sets[vk::SwapChain::currentFrame],
    topographySampler.Sets[vk::SwapChain::currentFrame],
    heightMap.Sets[vk::SwapChain::currentFrame]
};
std::vector<VkDescriptorSetLayout> planeLayout {
    ubo.SetLayout,
    modMat.SetLayout,
    topographySampler.SetLayout,
    heightMap.SetLayout
};
//
//vk::Descriptor planeDescriptors[] = {
//    ubo, modMat, topographySampler, heightMap
//};

vk::Shader planeShaders[] = {
    {"plane.vert", VK_SHADER_STAGE_VERTEX_BIT},
    {"plane.frag", VK_SHADER_STAGE_FRAGMENT_BIT}
};
vk::GraphicsPPL<triangleList, VK_POLYGON_MODE_LINE> planePPL(planeShaders, planeSet, planeLayout);

vk::Octree tree(vertices, 0.01f);
vk::SSBO modelSSBO(tree.matrices, VK_SHADER_STAGE_VERTEX_BIT);

std::vector<VkDescriptorSet> descSet2{
    ubo.Sets[vk::SwapChain::currentFrame],
    modelSSBO.Sets[vk::SwapChain::currentFrame]
};
std::vector<VkDescriptorSetLayout> SetLayouts_2{
    ubo.SetLayout,
    modelSSBO.SetLayout
};
vk::Shader octreeShaders[] = {
    {"instanced.vert", VK_SHADER_STAGE_VERTEX_BIT},
    {"instanced.frag", VK_SHADER_STAGE_FRAGMENT_BIT}
};
vk::GraphicsPPL<lineList> octreePPL(octreeShaders, descSet2, SetLayouts_2);//descSet2

std::vector<VkDescriptorSet> pointSet{
    ubo.Sets[vk::SwapChain::currentFrame],
    ssbo.Sets[vk::SwapChain::currentFrame]
};
std::vector<VkDescriptorSetLayout> pointLayout{
    ubo.SetLayout,
    ssbo.SetLayout
};
vk::Shader pointShaders[] = {
    {"point.vert", VK_SHADER_STAGE_VERTEX_BIT}, 
    {"point.frag", VK_SHADER_STAGE_FRAGMENT_BIT} 
};
vk::GraphicsPPL<Particle, VK_POLYGON_MODE_POINT> particlePPL(pointShaders, pointSet, pointLayout);

vk::Scene world[] = {
    {planePPL, plane},
    {octreePPL, tree.rootNode}
};

vk::Shader imageCompute("plane.comp", VK_SHADER_STAGE_COMPUTE_BIT);
vk::Shader particleCompute("point.comp", VK_SHADER_STAGE_COMPUTE_BIT);
vk::ComputePPL computePPL[] = {
    {imageCompute, planeSet, planeLayout},
    {particleCompute, pointSet, pointLayout}
};
//TODO: Create compute shader that generates a texture/image
//TODO: Make copy and move operators for pretty much everything

//TODO: Learn discrete differential geometry
//TODO: Implement waves~
//TODO: Optimize the swapchain and rendering process
//TODO: Optimize a fuckload of stuff with shader caching, pipeline caching, parallelization, etc.

VkPhysicalDeviceProperties properties;

int main() {
    //TODO: Make physical device properties file
    vkGetPhysicalDeviceProperties(vk::GPU::physicalDevice, &properties);
    try {
        while (!glfwWindowShouldClose(vk::Window::handle)) {
            glfwPollEvents();
            std::jthread t1(glfwSetKeyCallback, vk::Window::handle, vk::userInput);
            
            //tree.updateTree(vertices);
            ubo.update(uniforms);
            app.run(world, computePPL, particlePPL, ssbo);
            
        }
        vkDeviceWaitIdle(vk::GPU::device);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
