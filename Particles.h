#pragma once

#include "vk.ubo.h"
#include "vk.ssbo.h"
#include "vk.graphics.h"

pop population(100000);

vk::SSBO ssbo(population.particles, VK_SHADER_STAGE_COMPUTE_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

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


