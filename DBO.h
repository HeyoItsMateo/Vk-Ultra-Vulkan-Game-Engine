#ifndef hDBO
#define hDBO

struct memBuffer : VkCPU {
    VkBuffer mBuffer, sBuffer;
    VkDeviceMemory mMemory, sMemory;
    memBuffer(VkDeviceSize size) {
        std::thread t1(&memBuffer::createBuffer, this, std::ref(mBuffer), size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        std::thread t2(&memBuffer::createBuffer, this, std::ref(sBuffer), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        t1.join();
        t2.join();

        t1 = std::thread(&memBuffer::allocateMemory, this, std::ref(mBuffer), std::ref(mMemory), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        t2 = std::thread(&memBuffer::allocateMemory, this, std::ref(sBuffer), std::ref(sMemory), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        t1.join();
        t2.join();
    }
    ~memBuffer() {
        std::thread t1(vkDestroyBuffer, VkGPU::device, mBuffer, nullptr);
        std::thread t2(vkDestroyBuffer, VkGPU::device, sBuffer, nullptr);
        t1.join();
        t2.join();
        //vkDestroyBuffer(pGPU->device, sBuffer, nullptr);
        t1 = std::thread(vkFreeMemory, VkGPU::device, mMemory, nullptr);
        t2 = std::thread(vkFreeMemory, VkGPU::device, sMemory, nullptr);
        t1.join();
        t2.join();
    }
protected:
    void copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }
private:
    void createBuffer(VkBuffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage) {
        VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(VkGPU::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }
    }
    void allocateMemory(VkBuffer& buffer, VkDeviceMemory& memory, VkMemoryPropertyFlags properties) {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(VkGPU::device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VkGPU::findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(VkGPU::device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(VkGPU::device, buffer, memory, 0);
    }
};

template<typename T>
struct VkBufferObject : memBuffer {
    VkBufferObject(std::vector<T> const& content)
        : memBuffer(sizeof(T)*content.size())
    {
        VkDeviceSize bufferSize = sizeof(T) * content.size();

        void* data;
        vkMapMemory(VkGPU::device, sMemory, 0, bufferSize, 0, &data);
        memcpy(data, content.data(), (size_t)bufferSize);
        
        copyBuffer(sBuffer, mBuffer, bufferSize);
    }
    ~VkBufferObject() {
        vkUnmapMemory(VkGPU::device, sMemory);
    }
};

struct VkDescriptor {
    VkDescriptorSetLayout SetLayout;
    std::vector<VkDescriptorSet> Sets;
protected:
    VkDescriptorPool Pool;
    void createDescriptorSetLayout(VkDescriptorType type, VkShaderStageFlagBits flag) {

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            bindings.push_back(VkUtils::bindSetLayout(i, type, flag));
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(VkGPU::device, &layoutInfo, nullptr, &SetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }
    void createDescriptorPool(VkDescriptorType type) {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            poolSizes.push_back({ type, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) });
        }
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(VkGPU::device, &poolInfo, nullptr, &Pool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }
};

template<typename T>
struct VkDataBuffer : VkCPU, VkDescriptor {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    std::vector<VkBuffer> Buffer;
    std::vector<VkDeviceMemory> Memory;

    VkDeviceSize bufferSize;



    VkDataBuffer(T& ubo, VkDescriptorType type, VkShaderStageFlagBits flag) {
        createDataBuffer(ubo);

        createDescriptorSetLayout(type, flag);
        createDescriptorPool(type);
        createDescriptorSets(type);
    }
    ~VkDataBuffer() {
        vkUnmapMemory(VkGPU::device, stagingBufferMemory);
        vkDestroyBuffer(VkGPU::device, stagingBuffer, nullptr);
        vkFreeMemory(VkGPU::device, stagingBufferMemory, nullptr);

        std::for_each(std::execution::par, Buffer.begin(), Buffer.end(), 
            [&](VkBuffer buffer) { vkDestroyBuffer(VkGPU::device, buffer, nullptr); });

        std::for_each(std::execution::par, Memory.begin(), Memory.end(),
            [&](VkDeviceMemory memory) { vkFreeMemory(VkGPU::device, memory, nullptr); });

        vkDestroyDescriptorPool(VkGPU::device, Pool, nullptr);
        vkDestroyDescriptorSetLayout(VkGPU::device, SetLayout, nullptr);
    }

    void update(uint32_t currentImage, void* contents) {
        memcpy(data, contents, (size_t)bufferSize);

        copyBuffer(stagingBuffer, Buffer[currentImage], bufferSize);
    }
private:
    void* data;
    void constexpr createDataBuffer(T& ubo) {
        Buffer.resize(MAX_FRAMES_IN_FLIGHT);
        Memory.resize(MAX_FRAMES_IN_FLIGHT);

        bufferSize = sizeof(T);

        createBuffer(stagingBuffer, stagingBufferMemory, bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(VkGPU::device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, &ubo, (size_t)bufferSize);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(Buffer[i], Memory[i], bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            copyBuffer(stagingBuffer, Buffer[i], bufferSize);
        }
    }
    void constexpr createBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(VkGPU::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(VkGPU::device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VkGPU::findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(VkGPU::device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(VkGPU::device, buffer, memory, 0);
    }
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void createDescriptorSets(VkDescriptorType type) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, SetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = Pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        Sets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(VkGPU::device, &allocInfo, Sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = Buffer[i];
            bufferInfo.offset = 0;
            bufferInfo.range = bufferSize;

            VkWriteDescriptorSet writeUBO = VkUtils::writeDescriptor(0, type, Sets[i]);
            writeUBO.pBufferInfo = &bufferInfo;
            descriptorWrites[i] = writeUBO;
        }
        vkUpdateDescriptorSets(VkGPU::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
};

struct VkTextureSet : VkDescriptor {
    VkDescriptorType mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkShaderStageFlagBits mFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkTextureSet(VkTexture& texture) {
        createDescriptorSetLayout(mType, mFlag);
        createDescriptorPool(mType);
        createDescriptorSets(texture);
    }
    ~VkTextureSet() {
        vkDestroyDescriptorPool(VkGPU::device, Pool, nullptr);
        vkDestroyDescriptorSetLayout(VkGPU::device, SetLayout, nullptr);
    }

private:
    void createDescriptorSets(VkTexture& texture) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, SetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = Pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        Sets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(VkGPU::device, &allocInfo, Sets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = texture.ImageView; // TODO: Generalize
            imageInfo.sampler = texture.Sampler; // TODO: Generalize

            VkWriteDescriptorSet writeSampler = VkUtils::writeDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Sets[i]);
            writeSampler.pImageInfo = &imageInfo;
            descriptorWrites[i] = writeSampler;
        }
        vkUpdateDescriptorSets(VkGPU::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
};

#endif