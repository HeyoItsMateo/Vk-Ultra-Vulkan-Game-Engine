#ifndef hSSBO
#define hSSBO

const uint32_t PARTICLE_COUNT = 1000;

struct Particle {
    glm::vec4 position;
    glm::vec4 color;
    glm::vec4 velocity;

    const static VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    static VkVertexInputBindingDescription vkCreateBindings() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> vkCreateAttributes() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, color);

        return attributeDescriptions;
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
                std::uniform_real_distribution<float> rndDist(-0.9f, 0.9f);
                std::uniform_real_distribution<float> rndColor(0.f, 1.0f);

                float x = rndDist(rndEngine);
                float y = rndDist(rndEngine);
                float z = rndDist(rndEngine);
                particle.position = glm::vec4(x, y, z, 0.f);
                particle.velocity = glm::vec4(0, 0, 0, 0.f);
                particle.color = glm::vec4(rndColor(rndEngine), rndColor(rndEngine), rndColor(rndEngine), 1.f);
            });
    }
};

template<typename T>
struct SSBO : VkCommand, VkDescriptor {
    std::vector<T> bufferData;
    std::vector<VkBuffer> Buffer;

    template<typename Q>
    inline SSBO(Q& input, VkShaderStageFlags flags, VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
        bufferData = input;
        loadData();

        createDescriptorSetLayout(type, flags);
        createDescriptorPool(type);
        createDescriptorSets(type);
    }
    ~SSBO() {
        std::for_each(std::execution::par, Buffer.begin(), Buffer.end(),
            [&](VkBuffer buffer) { vkDestroyBuffer(VkGPU::device, buffer, nullptr); });

        std::for_each(std::execution::par, Memory.begin(), Memory.end(),
            [&](VkDeviceMemory memory) { vkFreeMemory(VkGPU::device, memory, nullptr); });

        vkDestroyDescriptorPool(VkGPU::device, Pool, nullptr);
        vkDestroyDescriptorSetLayout(VkGPU::device, SetLayout, nullptr);
    }
private:
    VkDeviceSize bufferSize;
    std::vector<VkDeviceMemory> Memory;
    
    void loadData() {
        void* data;
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        Buffer.resize(MAX_FRAMES_IN_FLIGHT);
        Memory.resize(MAX_FRAMES_IN_FLIGHT);

        bufferSize = sizeof(T) * bufferData.size();

        createBuffer(stagingBuffer, stagingBufferMemory,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(VkGPU::device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, bufferData.data(), (size_t)bufferSize);
        vkUnmapMemory(VkGPU::device, stagingBufferMemory);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(Buffer[i], Memory[i],
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            copyBuffer(stagingBuffer, Buffer[i]);
        }

        vkDestroyBuffer(VkGPU::device, stagingBuffer, nullptr);
        vkFreeMemory(VkGPU::device, stagingBufferMemory, nullptr);
    }
    void createBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VkBufferCreateInfo bufferInfo
        { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(VkGPU::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(VkGPU::device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VkGPU::findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(VkGPU::device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(VkGPU::device, buffer, memory, 0);
    }
    void copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer) {
        beginCommand();

        VkBufferCopy copyRegion{};
        copyRegion.size = bufferSize;
        vkCmdCopyBuffer(VkCommand::buffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endCommand();
    }
private:
    void createDescriptorSetLayout(VkDescriptorType& type, VkShaderStageFlags flags) {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(2);
        layoutBindings[0].binding = 0;
        layoutBindings[0].descriptorCount = 1;
        layoutBindings[0].descriptorType = type;
        layoutBindings[0].pImmutableSamplers = nullptr;
        layoutBindings[0].stageFlags = flags;

        layoutBindings[1].binding = 1;
        layoutBindings[1].descriptorCount = 1;
        layoutBindings[1].descriptorType = type;
        layoutBindings[1].pImmutableSamplers = nullptr;
        layoutBindings[1].stageFlags = flags;

        VkDescriptorSetLayoutCreateInfo layoutInfo
        { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
        layoutInfo.pBindings = layoutBindings.data();

        if (vkCreateDescriptorSetLayout(VkGPU::device, &layoutInfo, nullptr, &SetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor set layout!");
        }
    }
    void createDescriptorPool(VkDescriptorType& type) {
        VkDescriptorPoolSize poolSizes
        { type , static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2 };

        VkDescriptorPoolCreateInfo poolInfo
        { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSizes;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(VkGPU::device, &poolInfo, nullptr, &Pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }
    void createDescriptorSets(VkDescriptorType& type) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, SetLayout);
        VkDescriptorSetAllocateInfo allocInfo
        { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = Pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        Sets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(VkGPU::device, &allocInfo, Sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            VkDescriptorBufferInfo storageBufferInfoLastFrame{};
            storageBufferInfoLastFrame.buffer = Buffer[(i - 1) % MAX_FRAMES_IN_FLIGHT];
            storageBufferInfoLastFrame.offset = 0;
            storageBufferInfoLastFrame.range = bufferSize;

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = Sets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = type;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &storageBufferInfoLastFrame;

            VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
            storageBufferInfoCurrentFrame.buffer = Buffer[i];
            storageBufferInfoCurrentFrame.offset = 0;
            storageBufferInfoCurrentFrame.range = bufferSize;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = Sets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = type;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &storageBufferInfoCurrentFrame;

            vkUpdateDescriptorSets(VkGPU::device, 2, descriptorWrites.data(), 0, nullptr);
        }
    }
};

#endif