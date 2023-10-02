#pragma once
#ifndef hSSBO
#define hSSBO

#include <vector>
#include <random>
#include <glm/glm.hpp>
#include "vk.primitives.h"

struct pop {
    std::vector<Particle> particles;
    pop(int count) {
        particles.resize(count);
        std::random_device rand;
        std::mt19937_64 rndEngine(rand());
        for (auto& particle : particles) {
            std::uniform_real_distribution<float> rndDist(-0.79f, 0.79f);
            std::uniform_real_distribution<float> rndMass(0.01f, 0.25f);
            std::uniform_real_distribution<float> rndColor(0.1f, 1.0f);

            float x = rndDist(rndEngine);
            float y = rndDist(rndEngine);
            float z = rndDist(rndEngine);
            float w = rndMass(rndEngine);
            particle.position = glm::vec4(x, y, z, w);
            particle.velocity = glm::vec4(0);
            particle.color = glm::vec4(rndColor(rndEngine), rndColor(rndEngine), rndColor(rndEngine), 1.f);
        }
    }
};

#include "vk.gpu.h"
#include "vk.buffers.h"
#include "descriptors.h"

namespace vk {
    struct SSBO : DataBuffer, Descriptor {
        template<typename T>
        inline SSBO(T& bufferData, VkShaderStageFlags flags, uint32_t bindingCount = 2);
    public:
        void draw();
    private:
        int vertexCount = 0;
        void writeDescriptorSets(uint32_t bindingCount) override;
    };
}

#include "vk.ssbo.ipp"

#endif