#pragma once

#include "vk.ubo.h"
#include "vk.textures.h"
#include "vk.graphics.h"

#include "Mesh.h"
#include "Geometry.h"

vk::Geometry::Plane plane({ 300, 200 }, { 0.025, 0.025 });

vk::UBO modMat(plane.matrix, VK_SHADER_STAGE_VERTEX_BIT);
vk::ComputeImage heightMap({ 3000, 2000 }, VK_SHADER_STAGE_VERTEX_BIT);

std::vector<VkDescriptorSet> planeSet{
    ubo.Sets[vk::SwapChain::currentFrame],
    modMat.Sets[vk::SwapChain::currentFrame],
    heightMap.Sets[vk::SwapChain::currentFrame]
};
std::vector<VkDescriptorSetLayout> planeLayout{
    ubo.SetLayout,
    modMat.SetLayout,
    heightMap.SetLayout
};

vk::Shader planeShaders[] = {
    {"plane.vert", VK_SHADER_STAGE_VERTEX_BIT},
    {"plane.frag", VK_SHADER_STAGE_FRAGMENT_BIT}
};

vk::GraphicsPPL<triangleList, VK_POLYGON_MODE_LINE> planePPL(planeShaders, planeSet, planeLayout);