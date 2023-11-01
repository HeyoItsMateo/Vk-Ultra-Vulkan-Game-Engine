#pragma once

#include "Octree.h"
#include "vk.ubo.h"
#include "vk.ssbo.h"
#include "vk.textures.h"
#include "vk.graphics.h"

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
vk::Octree tree(vertices, 0.01f);

vk::Uniforms uniforms;
vk::UBO ubo(uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
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
vk::GraphicsPPL<lineList> octreePPL(octreeShaders, descSet2, SetLayouts_2);