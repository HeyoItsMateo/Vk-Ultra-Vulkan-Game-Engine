#ifndef hBufferObjects
#define hBufferObjects

struct VkBufferObject: VkCommand {
public:
    std::vector<VkBuffer> Buffer;
protected:
    VkDeviceSize bufferSize;
    std::vector<VkDeviceMemory> Memory;

    void createBuffer(VkBuffer& buffer, VkDeviceMemory& memory, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VkBufferCreateInfo bufferInfo
        { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(VkGPU::device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        createMemory(buffer, memory, properties);
    }
    void createMemory(VkBuffer& buffer, VkDeviceMemory& memory, VkMemoryPropertyFlags properties) {
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
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer) {
        beginCommand();

        VkBufferCopy copyRegion{};
        copyRegion.size = bufferSize;
        vkCmdCopyBuffer(VkCommand::buffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endCommand();
    }
};

struct UBO : VkBufferObject, VkDescriptor {
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    template<typename T>
    inline UBO(T& uniforms, VkShaderStageFlags flag, uint32_t bindingCount = 1)
        : VkDescriptor(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, flag, bindingCount)
    {
        loadData(uniforms);

        writeDescriptorSets(bindingCount);
    }
    ~UBO() {
        vkUnmapMemory(VkGPU::device, stagingBufferMemory);
        vkDestroyBuffer(VkGPU::device, stagingBuffer, nullptr);
        vkFreeMemory(VkGPU::device, stagingBufferMemory, nullptr);

        std::for_each(std::execution::par, Buffer.begin(), Buffer.end(),
            [&](VkBuffer buffer) { vkDestroyBuffer(VkGPU::device, buffer, nullptr); });

        std::for_each(std::execution::par, Memory.begin(), Memory.end(),
            [&](VkDeviceMemory memory) { vkFreeMemory(VkGPU::device, memory, nullptr); });
    }

    template <typename T>
    inline void update(T& uniforms) {
        uniforms.update(VkSwapChain::lastTime);

        memcpy(data, &uniforms, (size_t)bufferSize);

        copyBuffer(stagingBuffer, Buffer[VkSwapChain::currentFrame]);
    }
protected:
    template<typename T>
    inline void loadData(T& UBO) {
        Buffer.resize(MAX_FRAMES_IN_FLIGHT);
        Memory.resize(MAX_FRAMES_IN_FLIGHT);

        bufferSize = sizeof(T);

        createBuffer(stagingBuffer, stagingBufferMemory,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkMapMemory(VkGPU::device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, &UBO, (size_t)bufferSize);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(Buffer[i], Memory[i],
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            copyBuffer(stagingBuffer, Buffer[i]);
        }
    }
private:
    void* data;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    void writeDescriptorSets(uint32_t bindingCount) {
        std::vector<VkDescriptorBufferInfo> bufferInfo(bindingCount);
        std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            bufferInfo[0].buffer = Buffer[i];
    
            for (size_t j = 0; j < bindingCount; j++) {
                bufferInfo[j].offset = 0;
                bufferInfo[j].range = bufferSize;
                descriptorWrites[j] = writeSet(bufferInfo, { i, j });
            }
        }
        vkUpdateDescriptorSets(VkGPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
    }
};

struct SSBO : VkBufferObject, VkDescriptor {
    VkDescriptorType type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    template<typename T>
    inline SSBO(T& bufferData, VkShaderStageFlags flags, uint32_t bindingCount = 2)
        : VkDescriptor(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, flags, bindingCount)
    {
        loadData(bufferData);

        writeDescriptorSets(bindingCount);
    }
    ~SSBO() {
        std::for_each(std::execution::par, Buffer.begin(), Buffer.end(),
            [&](VkBuffer buffer) { vkDestroyBuffer(VkGPU::device, buffer, nullptr); });

        std::for_each(std::execution::par, Memory.begin(), Memory.end(),
            [&](VkDeviceMemory memory) { vkFreeMemory(VkGPU::device, memory, nullptr); });
    }
protected:
    template<typename T>
    inline void loadData(std::vector<T>& bufferData) {
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
private:
    void writeDescriptorSets(uint32_t bindingCount) {
        std::vector<VkDescriptorBufferInfo> bufferInfo(bindingCount);
        std::vector<VkWriteDescriptorSet> descriptorWrites(bindingCount);
        
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {           
            bufferInfo[0].buffer = Buffer[(i - 1) % MAX_FRAMES_IN_FLIGHT];
            bufferInfo[1].buffer = Buffer[i];

            for (size_t j = 0; j < bindingCount; j++) {
                bufferInfo[j].offset = 0;
                bufferInfo[j].range = bufferSize;

                descriptorWrites[j] = writeSet(bufferInfo, { i, j });
            }
            vkUpdateDescriptorSets(VkGPU::device, bindingCount, descriptorWrites.data(), 0, nullptr);
        }
    }
};


#endif