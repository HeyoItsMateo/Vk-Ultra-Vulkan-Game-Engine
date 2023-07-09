#ifndef hDBO
#define hDBO

#include <map>

struct VkContainer {
    VkBuffer mBuffer;
    VkDeviceSize mSize;
protected:
    VkDeviceMemory mMemory;
    VkBufferUsageFlags mUsage;
    VkMemoryPropertyFlags mProperties;
private:
    friend struct VkBufferMap;
};

enum BufferObject { VBO, EBO };

struct test {
    std::map<BufferObject, VkContainer> model;
    test() {
        model[VBO] = VkContainer{};
        model[EBO] = VkContainer{};

        model[VBO];
    }
private:
};

struct VkBufferMap {
    VkContainer VBO[2];
    VkContainer EBO[2];
    std::map<int, std::vector<VkContainer>> map { { 0, {}}, { 1, {} } };
    VkBufferMap(std::vector<Vertex> const& vtx, std::vector<uint16_t> const& idx) {
        VkCPU cpu[2];
        VkCommandBuffer cmdBuffer[2];
        initMap();
        initBuffers(vtx, idx);
        initCmdBuffers(cpu, cmdBuffer);
        submitCmdBuffers(cmdBuffer);

        vkFreeCommandBuffers(VkGPU::device, cpu[0].commandPool, 1, &cmdBuffer[0]);
        vkFreeCommandBuffers(VkGPU::device, cpu[1].commandPool, 1, &cmdBuffer[1]);
    }
    ~VkBufferMap() {
        unmapMemory();
        destroyBuffers();
        freeMemory();
    }
    void draw(VkCommandBuffer& commandBuffer) {
        // Vertex binding (objects to draw) and draw method
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &map[0][0].mBuffer, offsets);
        vkCmdBindIndexBuffer(commandBuffer, map[1][0].mBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(map[1][0].mSize / sizeof(uint16_t)), 1, 0, 0, 0);
    }
protected:
    void initMap() {
        VkContainer mBuffer[4];
        std::jthread t1([&] {map[0] = { mBuffer[0], mBuffer[1] }; });
        std::jthread t2([&] {map[1] = { mBuffer[2], mBuffer[3] }; });
    }
    void initBuffers(std::vector<Vertex> const& vtx, std::vector<uint16_t> const& idx) {
        std::jthread t1([&] {initBuffer<Vertex>(0, vtx, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT); });
        std::jthread t2([&] {initBuffer<uint16_t>(1, idx, VK_BUFFER_USAGE_INDEX_BUFFER_BIT); });
    }
    void initCmdBuffers(VkCPU cpu[], VkCommandBuffer cmdBuffer[]) {
        std::jthread t1([&] {writeCommand(0, cpu[0], cmdBuffer[0]); });
        std::jthread t2([&] {writeCommand(1, cpu[1], cmdBuffer[1]); });
    }
    void submitCmdBuffers(VkCommandBuffer* cmdBuffer) {
        VkSubmitInfo submitInfo
        { VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 2; //TODO: find out how to allocate two or more command buffers
        submitInfo.pCommandBuffers = cmdBuffer;

        vkQueueSubmit(VkGPU::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(VkGPU::graphicsQueue);
    }
private:   
    template<typename T>
    void initBuffer(int key, auto const& content, VkBufferUsageFlagBits usage) {
        VkDeviceSize size = sizeof(T) * content.size();
        std::jthread t1 ([&] 
            {// Create main buffers for VBO and EBO
                map[key][0].mSize = size;
                map[key][0].mUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
                map[key][0].mProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                createBuffer(map[key][0]);
                allocateMemory(map[key][0]);
            }
        );
        std::jthread t2 ([&] 
            {// Create staging buffers for VBO or EBO
                map[key][1].mSize = size;
                map[key][1].mUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                map[key][1].mProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                createBuffer(map[key][1]);
                allocateMemory(map[key][1]);
                stageData(map[key][1], content.data());
            }
        );
    }
    void createBuffer(VkContainer& container) {
        VkBufferCreateInfo bufferInfo
        { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = container.mSize;
        bufferInfo.usage = container.mUsage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(VkGPU::device, &bufferInfo, nullptr, &container.mBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }
    }
    void allocateMemory(VkContainer& container) {
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(VkGPU::device, container.mBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VkGPU::findMemoryType(memRequirements.memoryTypeBits, container.mProperties);

        if (vkAllocateMemory(VkGPU::device, &allocInfo, nullptr, &container.mMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(VkGPU::device, container.mBuffer, container.mMemory, 0);
    }
    void stageData(VkContainer& container, const void* content)
    {// Only for Staging Buffers
        void* data;
        vkMapMemory(VkGPU::device, container.mMemory, 0, container.mSize, 0, &data);
        memcpy(data, content, (size_t)container.mSize);
    }

    void writeCommand(int key, VkCPU& CPU, VkCommandBuffer& commandBuffer) {
        CPU.beginSingleTimeCommands(commandBuffer);

        VkBufferCopy copyRegion{};
        copyRegion.size = map[key][0].mSize;
        vkCmdCopyBuffer(commandBuffer, map[key][1].mBuffer, map[key][0].mBuffer, 1, &copyRegion);
        vkEndCommandBuffer(commandBuffer);
    }

    void unmapMemory() {
        std::jthread tS1(vkUnmapMemory, VkGPU::device, map[0][1].mMemory);
        std::jthread tS2(vkUnmapMemory, VkGPU::device, map[1][1].mMemory);
    }
    void destroyBuffers() {
        std::jthread t1(vkDestroyBuffer, VkGPU::device, map[0][0].mBuffer, nullptr);
        std::jthread t2(vkDestroyBuffer, VkGPU::device, map[0][1].mBuffer, nullptr);
        std::jthread t3(vkDestroyBuffer, VkGPU::device, map[1][0].mBuffer, nullptr);
        std::jthread t4(vkDestroyBuffer, VkGPU::device, map[1][1].mBuffer, nullptr);
    }
    void freeMemory() {
        std::jthread t1(vkFreeMemory, VkGPU::device, map[0][0].mMemory, nullptr);
        std::jthread t2(vkFreeMemory, VkGPU::device, map[0][1].mMemory, nullptr);
        std::jthread t3(vkFreeMemory, VkGPU::device, map[1][0].mMemory, nullptr);
        std::jthread t4(vkFreeMemory, VkGPU::device, map[1][1].mMemory, nullptr);
    }
};

struct VkDescriptor {
    VkDescriptorSetLayout SetLayout;
    std::vector<VkDescriptorSet> Sets;
protected:
    VkDescriptorPool Pool;
    void createDescriptorSetLayout(VkDescriptorType type, VkShaderStageFlags flag) {

        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            bindings.push_back(bindSetLayout(i, type, flag));
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
private:
    static VkDescriptorSetLayoutBinding bindSetLayout(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags) {
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.pImmutableSamplers = nullptr;
        layoutBinding.stageFlags = stageFlags;
        return layoutBinding;
    }
};

template<typename T>
struct VkUniformBuffer : VkCPU, VkDescriptor {
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    std::vector<VkBuffer> Buffer;
    std::vector<VkDeviceMemory> Memory;

    VkDeviceSize bufferSize;

    constexpr VkUniformBuffer(T& ubo, VkDescriptorType type, VkShaderStageFlags flag) {
        createDataBuffer(ubo);

        createDescriptorSetLayout(type, flag);
        createDescriptorPool(type);
        createDescriptorSets(type);
    }
    ~VkUniformBuffer() {
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
        VkCommandBuffer commandBuffer;
        beginSingleTimeCommands(commandBuffer);

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