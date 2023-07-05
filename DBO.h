#ifndef hDBO
#define hDBO

struct VkBufferObject : VkCPU {
    VkBuffer VBO, EBO, sVBO, sEBO;
    VkDeviceMemory mVBO, mEBO, smVBO, smEBO;
    VkDeviceSize sizeVBO, sizeEBO;
    VkBufferObject(std::vector<Vertex> const& content, std::vector<uint16_t> const& indices) {
        std::thread tVBO(&VkBufferObject::getSize<Vertex>, this, std::ref(content), std::ref(sizeVBO));
        std::thread tEBO(&VkBufferObject::getSize<uint16_t>, this, std::ref(indices), std::ref(sizeEBO));
        tVBO.join(); tEBO.join();

        tVBO = std::thread(&VkBufferObject::createBuffer, this, std::ref(VBO), sizeVBO, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        tEBO = std::thread(&VkBufferObject::createBuffer, this, std::ref(EBO), sizeEBO, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
        std::thread tsVBO(&VkBufferObject::createBuffer, this, std::ref(sVBO), sizeVBO, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        std::thread tsEBO(&VkBufferObject::createBuffer, this, std::ref(sEBO), sizeEBO, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        tVBO.join(); tEBO.join(); tsVBO.join(); tsEBO.join();

        tVBO = std::thread(&VkBufferObject::allocateMemory, this, std::ref(VBO), std::ref(mVBO), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        tEBO = std::thread(&VkBufferObject::allocateMemory, this, std::ref(EBO), std::ref(mEBO), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        tsVBO = std::thread(&VkBufferObject::allocateMemory, this, std::ref(sVBO), std::ref(smVBO), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        tsEBO = std::thread(&VkBufferObject::allocateMemory, this, std::ref(sEBO), std::ref(smEBO), VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        tVBO.join(); tEBO.join(); tsVBO.join(); tsEBO.join();

        tVBO = std::thread(&VkBufferObject::setData<Vertex>, this, std::ref(content), std::ref(VBO), std::ref(sVBO), std::ref(smVBO), sizeVBO);
        tEBO = std::thread(&VkBufferObject::setData<uint16_t>, this, std::ref(indices), std::ref(EBO), std::ref(sEBO), std::ref(smEBO), sizeEBO);
        tVBO.join(); tEBO.join();
    }
    ~VkBufferObject() {
        std::thread tsVBO(vkUnmapMemory, VkGPU::device, smVBO);
        std::thread tsEBO(vkUnmapMemory, VkGPU::device, smEBO);
        tsVBO.join(); tsEBO.join();
        
        std::thread tVBO(vkDestroyBuffer, VkGPU::device, VBO, nullptr);
        std::thread tEBO(vkDestroyBuffer, VkGPU::device, EBO, nullptr);
        tsVBO = std::thread(vkDestroyBuffer, VkGPU::device, sVBO, nullptr);
        tsEBO = std::thread(vkDestroyBuffer, VkGPU::device, sEBO, nullptr);
        tVBO.join(); tEBO.join(); tsVBO.join(); tsEBO.join();
        
        //vkDestroyBuffer(pGPU->device, sBuffer, nullptr);
        tVBO = std::thread(vkFreeMemory, VkGPU::device, mVBO, nullptr);
        tEBO = std::thread(vkFreeMemory, VkGPU::device, mEBO, nullptr);
        tsVBO = std::thread(vkFreeMemory, VkGPU::device, smVBO, nullptr);
        tsEBO = std::thread(vkFreeMemory, VkGPU::device, smEBO, nullptr);
        tVBO.join(); tEBO.join(); tsVBO.join(); tsEBO.join();
    }
    void draw(VkCommandBuffer commandBuffer) {
        // Vertex binding (objects to draw) and draw method
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &VBO, offsets);
        vkCmdBindIndexBuffer(commandBuffer, EBO, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(sizeEBO / sizeof(uint16_t)), 1, 0, 0, 0);
    }
private:
    template<typename T>
    void getSize(std::vector<T> const& content, VkDeviceSize& size) {
        size = sizeof(T) * content.size();
    }
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
    
    template<typename T>
    void setData(std::vector<T> const& content, VkBuffer& mBuffer, VkBuffer& sBuffer, VkDeviceMemory& sMemory, VkDeviceSize size) {
        void* data;
        vkMapMemory(VkGPU::device, sMemory, 0, size, 0, &data);
        memcpy(data, content.data(), (size_t)size);

        copyBuffer(sBuffer, mBuffer, size);
    }
    //TODO: Fix threading error when calling commandBuffers
    //Method: Create commandBuffers in threads, then submit command buffers together after joining threads
    void copyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
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

    constexpr VkDataBuffer(T& ubo, VkDescriptorType type, VkShaderStageFlagBits flag) {
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
    void createDataBuffer(T& ubo) {
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
    void createBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
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