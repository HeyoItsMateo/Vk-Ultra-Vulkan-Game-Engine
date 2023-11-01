#pragma once

#include "Model.h"
#include "vk.ubo.h"
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
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};
vk::Mesh model(vertices, indices);

vk::Uniforms uniforms;

vk::UBO ubo(uniforms, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);
vk::Texture planks("textures/planks.png");
vk::Sampler sampler(planks.mipLevels);
//vk::CombinedImageSampler planksImageSampler("textures/planks.png");

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

vk::Shader plankShaders[] = {
    {"vertex.vert", VK_SHADER_STAGE_VERTEX_BIT},
    {"vertex.frag", VK_SHADER_STAGE_FRAGMENT_BIT}
};
vk::GraphicsPPL<triangleList> graphicsPPL(plankShaders, plankSet, plankLayout);