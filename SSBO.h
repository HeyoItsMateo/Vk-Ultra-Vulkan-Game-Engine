#ifndef hSSBO
#define hSSBO

const uint32_t PARTICLE_COUNT = 1000;

struct Particle {
    glm::vec4 position;
    glm::vec4 color;
    glm::vec4 velocity;

    //const static VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    static VkVertexInputBindingDescription vkCreateBindings() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> vkCreateAttributes() {
        std::vector<VkVertexInputAttributeDescription> Attributes{
            { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, position)}, // Position
            { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Particle, color) },  // Color
        };
        return Attributes;
    }
    static VkPipelineVertexInputStateCreateInfo vkCreateVertexInput() {
        static auto bindingDescription = vkCreateBindings();
        static auto attributeDescriptions = vkCreateAttributes();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        return vertexInputInfo;
    }
};

struct pop {
    std::vector<Particle> particles;
    pop(int PARTICLE_COUNT = PARTICLE_COUNT) {
        particles.resize(PARTICLE_COUNT);
        std::random_device rand;
        std::mt19937_64 rndEngine(rand());
        std::for_each(std::execution::par, particles.begin(), particles.end(),
            [&](Particle& particle)
            {
                std::uniform_real_distribution<float> rndDist(-0.79f, 0.79f);
                std::uniform_real_distribution<float> rndColor(0.1f, 1.0f);

                float x = rndDist(rndEngine);
                float y = rndDist(rndEngine);
                float z = rndDist(rndEngine);
                particle.position = glm::vec4(x, y, z, 1.f);
                particle.velocity = glm::vec4(0);
                particle.color = glm::vec4(rndColor(rndEngine), rndColor(rndEngine), rndColor(rndEngine), 1.f);
            });
    }
};

#endif