#pragma once
#include <vector>
#include <random>
#include <glm/glm.hpp>

#include "vk.graphics.h"
#include "vk.primitives.h"

#include "vk.ubo.h"
#include "vk.ssbo.h"

#include "ProcGenLib.h"

struct particleSystem {
    std::vector<Particle> particles;
    particleSystem(int count)
    {
        particles.resize(count);
        for (auto& particle : particles) {
            std::uniform_real_distribution<float> rndDist(-0.79f, 0.79f);
            std::uniform_real_distribution<float> rndMass(0.01f, 0.25f);
            std::uniform_real_distribution<float> rndColor(0.1f, 1.0f);

            float x = rndDist(pgl::global_rng);
            float y = rndDist(pgl::global_rng);
            float z = rndDist(pgl::global_rng);
            float w = rndMass(pgl::global_rng);
            particle.position = glm::vec4(x, y, z, w);
            particle.velocity = glm::vec4(0);
            particle.color = glm::vec4(rndColor(pgl::global_rng), rndColor(pgl::global_rng), rndColor(pgl::global_rng), 1.f);
        }
    }
};

particleSystem stars(100000);
vk::SSBO ssbo(stars.particles, VK_SHADER_STAGE_COMPUTE_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);


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


