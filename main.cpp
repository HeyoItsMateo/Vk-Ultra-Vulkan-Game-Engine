#include "vk.engine.h"

#include <algorithm>
#include <functional>

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

vk::Window window("Vulkan");
vk::Engine app;

pop population;
vk::SSBO ssbo(population.particles, VK_SHADER_STAGE_COMPUTE_BIT);

std::vector<Vertex> vertices = {
    {{-0.5f,  0.5f,  0.5f, 1.f}, {1.0f, 0.0f, 0.0f, 1.f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f, 1.f}, {0.0f, 1.0f, 0.0f, 1.f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f, 1.f}, {0.0f, 0.0f, 1.0f, 1.f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f, 1.f}, {1.0f, 1.0f, 1.0f, 1.f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f,  0.5f, 1.f}, {1.0f, 0.0f, 0.0f, 1.f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f, 1.f}, {0.0f, 1.0f, 0.0f, 1.f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f, 1.f}, {0.0f, 0.0f, 1.0f, 1.f}, {1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f, 1.f}, {1.0f, 1.0f, 1.0f, 1.f}, {0.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

vk::PhxModel model(vertices, indices);

Octree tree(vertices, 0.01f);
vk::SSBO modelSSBO(tree.matrices, VK_SHADER_STAGE_VERTEX_BIT);

vk::Uniforms uniforms;
vk::UBO ubo(uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

vk::Texture planks("textures/planks.png");
//std::vector<VkTexture> textures{planks};
vk::TextureSet textureSet(planks);

std::vector<VkDescriptorSet> descSet1 = { ubo.Sets[vk::SwapChain::currentFrame], textureSet.Sets[vk::SwapChain::currentFrame], ssbo.Sets[vk::SwapChain::currentFrame] };
std::vector<VkDescriptorSetLayout> testing = { ubo.SetLayout, textureSet.SetLayout, ssbo.SetLayout };

std::vector<VkDescriptorSet> descSet2 = { ubo.Sets[vk::SwapChain::currentFrame], textureSet.Sets[vk::SwapChain::currentFrame], modelSSBO.Sets[vk::SwapChain::currentFrame] };
std::vector<VkDescriptorSetLayout> testing2 = { ubo.SetLayout, textureSet.SetLayout, modelSSBO.SetLayout };

vk::Shader vertShader("vertex.vert", VK_SHADER_STAGE_VERTEX_BIT);
vk::Shader fragShader("vertex.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
vk::Shader compShader("shader.comp", VK_SHADER_STAGE_COMPUTE_BIT);
std::vector<vk::Shader*> shaders = { &vertShader, &fragShader };

vk::Shader pointVert("point.vert", VK_SHADER_STAGE_VERTEX_BIT);
vk::Shader pointFrag("point.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
std::vector<vk::Shader*> compShaders = { &pointVert, &pointFrag };

vk::Shader octreeVert("octree.vert", VK_SHADER_STAGE_VERTEX_BIT);
vk::Shader octreeFrag("octree.frag", VK_SHADER_STAGE_FRAGMENT_BIT);
std::vector<vk::Shader*> octreeShaders = { &octreeVert, &octreeFrag };

std::vector<vk::Shader> testShaders_default {
    { "vertex.vert", VK_SHADER_STAGE_VERTEX_BIT },
    { "vertex.frag", VK_SHADER_STAGE_FRAGMENT_BIT },
    { "shader.comp", VK_SHADER_STAGE_COMPUTE_BIT }
};

std::vector<vk::Shader> testShaders_points {
    { "point.vert", VK_SHADER_STAGE_VERTEX_BIT },
    { "point.frag", VK_SHADER_STAGE_FRAGMENT_BIT }
};

std::vector<vk::Shader> testShaders_octree { 
    { "octree.vert", VK_SHADER_STAGE_VERTEX_BIT },
    { "octree.frag", VK_SHADER_STAGE_FRAGMENT_BIT } 
};

vk::GraphicsPipeline<Vertex> pipeline(descSet1, shaders, testing);

vk::GraphicsPipeline<Voxel> octreePPL(descSet2, octreeShaders, testing2);

vk::ParticlePipeline particlePPL(descSet1, compShaders, testing);

vk::ComputePipeline computePPL(descSet1, compShader.stageInfo, testing);


//TODO: Learn discrete differential geometry
//TODO: Implement waves~
//TODO: Optimize the swapchain and rendering process
//TODO: Optimize a fuckload of stuff with shader caching, pipeline caching, parallelization, etc.

VkPhysicalDeviceProperties properties;

int main() {
    

    vkGetPhysicalDeviceProperties(vk::GPU::physicalDevice, &properties);

    std::printf("maxWorkgroupSize is: %i \n", *properties.limits.maxComputeWorkGroupSize);
    std::printf("maxWorkGroupInvocations is: %i \n", properties.limits.maxComputeWorkGroupInvocations);
    std::printf("maxWorkGroupCount is: %i \n", *properties.limits.maxComputeWorkGroupCount);

    try {
        //runProcess("shader.comp");
        app.run(pipeline, octreePPL, model, tree, particlePPL, computePPL, ssbo, ubo, uniforms);
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
